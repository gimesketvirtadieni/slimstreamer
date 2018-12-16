/*
 * Copyright 2017, Andrej Kislovskij
 *
 * This is PUBLIC DOMAIN software so use at your own risk as it comes
 * with no warranties. This code is yours to share, use and modify without
 * any restrictions or obligations.
 *
 * For more information see conwrap/LICENSE or refer refer to http://unlicense.org
 *
 * Author: gimesketvirtadieni at gmail dot com (Andrej Kislovskij)
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <conwrap2/ProcessorProxy.hpp>
#include <cstddef>  // std::size_t
#include <cstdint>  // std::u..._t types
#include <functional>
#include <memory>
#include <sstream>  // std::stringstream
#include <string>
#include <type_safe/optional.hpp>
#include <type_safe/optional_ref.hpp>
#include <unordered_map>
#include <vector>

#include "slim/Chunk.hpp"
#include "slim/Consumer.hpp"
#include "slim/ContainerBase.hpp"
#include "slim/EncoderBuilder.hpp"
#include "slim/Exception.hpp"
#include "slim/log/log.hpp"
#include "slim/proto/Command.hpp"
#include "slim/proto/CommandSession.hpp"
#include "slim/proto/StreamingSession.hpp"
#include "slim/util/BigInteger.hpp"
#include "slim/util/StateMachine.hpp"
#include "slim/util/Timestamp.hpp"


namespace slim
{
	namespace proto
	{
		namespace ts = type_safe;

		template<typename ConnectionType>
		class Streamer : public Consumer
		{
			template<typename SessionType>
			using SessionsMap          = std::unordered_map<ConnectionType*, std::unique_ptr<SessionType>>;
			using CommandSessionType   = CommandSession<ConnectionType>;
			using StreamingSessionType = StreamingSession<ConnectionType>;

			enum Event
			{
				StreamEvent,
				StopEvent,
			};

			enum State
			{
				ReadyState,
				BufferingState,
			};

			public:
				Streamer(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pp, unsigned int sp, EncoderBuilder eb, std::optional<unsigned int> ga)
				: Consumer{pp}
				, streamingPort{sp}
				, encoderBuilder{eb}
				, gain{ga}
				, stateMachine
				{
					ReadyState,  // initial state
					{   // transition table definition
						{StreamEvent, ReadyState,     BufferingState, [&](auto event) {LOG(DEBUG) << "BUFFER";stateChangeToBuffering();}, [&] {return true;}},
						{StreamEvent, ReadyState,     ReadyState,     [&](auto event) {}, [&] {return true;}},
						{StopEvent,   BufferingState, ReadyState,     [&](auto event) {LOG(DEBUG) << "READY";stateChangeToReady();}, [&] {return true;}},
						{StopEvent,   ReadyState,     ReadyState,     [&](auto event) {}, [&] {return true;}},
					}
				}
				{
					LOG(DEBUG) << LABELS{"proto"} << "Streamer object was created (id=" << this << ")";
				}

				virtual ~Streamer()
				{
					LOG(DEBUG) << LABELS{"proto"} << "Streamer object was deleted (id=" << this << ")";
				}

				// TODO: non-movable is required due to usage in server callbacks; consider refactoring
				Streamer(const Streamer&) = delete;             // non-copyable
				Streamer& operator=(const Streamer&) = delete;  // non-assignable
				Streamer(Streamer&& rhs) = delete;              // non-movable
				Streamer& operator=(Streamer&& rhs) = delete;   // non-movable-assinable

				virtual bool consumeChunk(Chunk& chunk) override
				{
					auto result{false};
					auto streamSamplingRate{getSamplingRate()};
					auto chunkSamplingRate{chunk.getSamplingRate()};

					// skipping chunks with sampling rate = 0
					if (!chunkSamplingRate)
					{
						LOG(WARNING) << LABELS{"proto"} << "Chunk was skipped due to invalid sampling rate (rate=0)";

						result = true;
					}

					// if this is the beginning of a stream then changing state to BufferingState
					if (chunkSamplingRate && !streamSamplingRate.has_value())
					{
						// assigning new sampling rate
						setSamplingRate(chunkSamplingRate);
						streamSamplingRate = chunkSamplingRate;

						stateMachine.processEvent(StreamEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Stream event - skipping chunk";
							result = true;
						});
					}

					// if streaming is ongoing (the most common case)
					if (chunkSamplingRate && chunkSamplingRate == streamSamplingRate.value_or(0))
					{
						// if buffering is still ongoing then ckecking if it's completed
						if (!initializingStream && streamingStartedAt.has_value() && !playbackStartedAt.has_value() && isReadyToPlay())
						{
							startPlayback();
						}

						// if initialization of a stream was done
						if (!initializingStream || isReadyToStream())
						{
							initializingStream = false;

							// this is the middle of a stream, so just distributing a chunk
							streamChunk(chunk);

							result = true;
						}
					}

					// TODO: consider DrainingState
					// if sampling rate is changing in the middle of the stream then a new stream must be initialized
					if (chunkSamplingRate && chunkSamplingRate != streamSamplingRate.value_or(chunkSamplingRate))
					{
						LOG(WARNING) << LABELS{"proto"} << "Sampling rate has changed in the middle of a stream (current rate=" << streamSamplingRate.value_or(0) << "; new rate=" << chunkSamplingRate << ")";

						stateMachine.processEvent(StopEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Stop event - skipping chunk";
							result = true;
						});

						// resetting samplingRate
						streamSamplingRate.reset();
						setSamplingRate(streamSamplingRate);
					}

					// TODO: consider DrainingState
					// if this is the end of a stream then changing state to ReadyState
					if (chunk.isEndOfStream())
					{
						stateMachine.processEvent(StopEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Stop event - skipping chunk";
							result = true;
						});

						// resetting samplingRate
						streamSamplingRate.reset();
						setSamplingRate(streamSamplingRate);
					}

					return result;
				}

				void onHTTPClose(ConnectionType& connection)
				{
					LOG(INFO) << LABELS{"proto"} << "HTTP close callback (connection=" << &connection << ")";

					auto streamingSessionPtr{removeSession(streamingSessions, connection)};
					if (streamingSessionPtr)
					{
						// if there is a relevant SlimProto connection found
						auto clientID{streamingSessionPtr->getClientID()};
						auto commandSession{findSessionByID(commandSessions, clientID)};
						if (commandSession.has_value())
						{
							// resetting HTTP session in its relevant SlimProto session
							commandSession.value()->setStreamingSession(ts::nullopt);
						}
						else
						{
							LOG(WARNING) << LABELS{"proto"} << "Could not find SlimProto session by client ID (clientID=" << clientID << ")";
						}
					}
					else
					{
						LOG(WARNING) << LABELS{"proto"} << "Could not find HTTP session object";
					}
				}

				void onHTTPData(ConnectionType& connection, unsigned char* buffer, std::size_t receivedSize)
				{
					if (!applyToSession(streamingSessions, connection, [&](StreamingSessionType& session)
					{
						// processing request by a proper Streaming session mapped to this connection
						session.onRequest(buffer, receivedSize);
					}))
					{
						// parsing client ID
						auto clientID = StreamingSessionType::parseClientID(std::string{(char*)buffer, receivedSize});
						if (!clientID.has_value())
						{
							throw slim::Exception("Missing client ID in streaming session request");
						}

						LOG(INFO) << LABELS{"proto"} << "Client ID was parsed (clientID=" << clientID.value() << ")";

						ts::with(getSamplingRate(), [&](auto samplingRate)
						{
							// setting encoder sampling rate
							encoderBuilder.setSamplingRate(samplingRate);

							// creating streaming session object
							auto streamingSessionPtr{std::make_unique<StreamingSessionType>(std::ref<ConnectionType>(connection), clientID.value(), encoderBuilder)};

							// saving Streaming session reference in the relevant Command session
							auto commandSession{findSessionByID(commandSessions, clientID.value())};
							if (!commandSession.has_value())
							{
								connection.stop();
								throw slim::Exception("Could not correlate provided client ID with a valid SlimProto session");
							}
							commandSession.value()->setStreamingSession(ts::optional_ref<StreamingSessionType>{*streamingSessionPtr});

							// processing request by a proper Streaming session mapped to this connection
							streamingSessionPtr->onRequest(buffer, receivedSize);

							// saving Streaming session as a part of this Streamer
							addSession(streamingSessions, connection, std::move(streamingSessionPtr));
						});
					}
				}

				void onHTTPOpen(ConnectionType& connection)
				{
					LOG(INFO) << LABELS{"proto"} << "New HTTP session request received (connection=" << &connection << ")";
				}

				void onSlimProtoClose(ConnectionType& connection)
				{
					removeSession(commandSessions, connection);
				}

				void onSlimProtoData(ConnectionType& connection, unsigned char* buffer, std::size_t size, util::Timestamp timestamp)
				{
					try
					{
						if (!applyToSession(commandSessions, connection, [&](CommandSessionType& session)
						{
							session.onRequest(buffer, size, timestamp);
						}))
						{
							throw slim::Exception("Could not find SlimProto session object");
						}
					}
					catch (const slim::Exception& error)
					{
						LOG(ERROR) << LABELS{"proto"} << "Error while processing SlimProto command: " << error.what();
						connection.stop();
					}
				}

				void onSlimProtoOpen(ConnectionType& connection)
				{
					// using regular counter for session ID's instead of MAC's; it allows running multiple players on one host
					std::stringstream ss;
					ss << (++nextID);

					// creating command session object
					auto commandSessionPtr{std::make_unique<CommandSessionType>(getProcessorProxy(), std::ref<ConnectionType>(connection), ss.str(), streamingPort, encoderBuilder.getFormat(), gain)};

					// enable streaming for this session if required
					ts::with(streamingStartedAt, [&](auto& streamingStartedAt)
					{
						ts::with(getSamplingRate(), [&](auto samplingRate)
						{
							commandSessionPtr->startStreaming(samplingRate, streamingStartedAt + getStreamingDuration(util::microseconds));
						});
					});

					// saving command session in the map
					addSession(commandSessions, connection, std::move(commandSessionPtr));
				}

				virtual void start() override {}
				virtual void stop(bool gracefully = true) override {}

			protected:
				template<typename SessionType>
				inline auto& addSession(SessionsMap<SessionType>& sessions, ConnectionType& connection, std::unique_ptr<SessionType> sessionPtr)
				{
					auto         found{sessions.find(&connection)};
					SessionType* s{nullptr};

					if (found == sessions.end())
					{
						s = sessionPtr.get();

						// saving session in a map; using pointer to a relevant connection as an ID
						sessions[&connection] = std::move(sessionPtr);
						LOG(DEBUG) << LABELS{"proto"} << "New session was added (id=" << s << ", sessions=" << sessions.size() << ")";
					}
					else
					{
						s = (*found).second.get();
						LOG(WARNING) << LABELS{"proto"} << "Session already exists";
					}

					return *s;
				}

				template<typename SessionType, typename FunctionType>
				inline bool applyToSession(SessionsMap<SessionType>& sessions, ConnectionType& connection, FunctionType fun)
				{
					auto found{sessions.find(&connection)};

					if (found != sessions.end())
					{
						fun(*(*found).second);
					}

					return (found != sessions.end());
				}

				inline auto calculatePlaybackTime()
				{
					auto maxLatency{std::chrono::microseconds{100}};
					
					for (auto& entry : commandSessions)
					{
						auto latency{entry.second->getLatency()};
						ts::with(latency, [&](auto& latency)
						{
							if (maxLatency.count() < latency.count())
							{
								maxLatency = latency;
							}
						});
					}

					// adding extra 5 milliseconds required for sending out command to all the clients
					return util::Timestamp::now() + maxLatency + std::chrono::milliseconds{5};
				}

				template<typename SessionType>
				auto findSessionByID(SessionsMap<SessionType>& sessions,  std::string clientID)
				{
					auto result{std::optional<SessionType*>{std::nullopt}};
					auto found{std::find_if(sessions.begin(), sessions.end(), [&](auto& entry) -> bool
					{
						auto found{false};

						if (entry.second->getClientID() == clientID)
						{
							found = true;
						}

						return found;
					})};

					if (found != sessions.end())
					{
						result = (*found).second.get();
					}

					return result;
				}

				template<typename RatioType>
				inline auto getStreamingDuration(RatioType ratio) const
 				{
					auto result{std::chrono::duration<long long, RatioType>{0}};

					ts::with(getSamplingRate(), [&](auto samplingRate)
					{
						result = std::chrono::duration<long long, RatioType>{streamingFrames * ratio.den / samplingRate};
					});

					return result;
 				}

				inline auto isReadyToPlay()
				{
					// TODO: introduce timeout threshold
					return commandSessions.size() == static_cast<unsigned int>(std::count_if(commandSessions.begin(), commandSessions.end(), [&](auto& entry)
					{
						return entry.second->isReadyToPlay();
					}));
				}

				inline auto isReadyToStream()
				{
					auto result{false};

					ts::with(streamingStartedAt, [&](auto& streamingStartedAt)
					{
						// TODO: deferring time-out should be configurable
						auto waitThresholdReached{std::chrono::milliseconds{500} < (util::Timestamp::now() - streamingStartedAt)};
						auto readyToStreamTotal{std::count_if(commandSessions.begin(), commandSessions.end(), [&](auto& entry)
						{
							return !entry.second->isReadyToStream();
						})};

						// if deferring time-out has expired
						if (waitThresholdReached)
						{
							result = true;

							if (readyToStreamTotal)
							{
								LOG(WARNING) << LABELS{"proto"} << "Could not defer chunk processing due to reached threshold";
							}
						}
						else if (!readyToStreamTotal)
						{
							// all SlimProto sessions are ready to stream
							result = true;
						}
					});

					return result;
				}

				template<typename SessionType>
				inline auto removeSession(SessionsMap<SessionType>& sessions, ConnectionType& connection)
				{
					std::unique_ptr<SessionType> sessionPtr{};
					auto                         found{sessions.find(&connection)};

					if (found != sessions.end())
					{
						sessionPtr = std::move((*found).second);
						sessions.erase(found);
						LOG(DEBUG) << LABELS{"proto"} << "Session was removed (id=" << sessionPtr.get() << ", sessions=" << sessions.size() << ")";
					}

					return std::move(sessionPtr);
				}

				inline auto startPlayback()
				{
					// capturing start playback time point
					playbackStartedAt = calculatePlaybackTime();

					ts::with(playbackStartedAt, [&](auto& playbackStartedAt)
					{
						for (auto& entry : commandSessions)
						{
							entry.second->startPlayback(playbackStartedAt);
						}
					});
				}

				inline void stateChangeToBuffering()
				{
					if (!streamingStartedAt.has_value())
					{
						ts::with(getSamplingRate(), [&](auto samplingRate)
						{
							// capturing start streaming time point (required for calculations like defer time-out, etc.)
							streamingStartedAt = util::Timestamp::now();
							streamingFrames    = 0;
							initializingStream = true;

							// TODO: consider isReadyToStart
							for (auto& entry : commandSessions)
							{
								entry.second->startStreaming(samplingRate, streamingStartedAt.value_or(util::Timestamp::now()));
							}

							LOG(DEBUG) << LABELS{"proto"} << "Started streaming (rate=" << samplingRate << ")";
						});
					}
				}

				inline void stateChangeToReady()
				{
					// stop streaming if streaming is ongoing 
					ts::with(streamingStartedAt, [&](auto& streamingStartedAt)
					{
						for (auto& entry : commandSessions)
						{
							// stopping SlimProto session will send end-of-stream command which normally triggers close of HTTP connection
							entry.second->stopStreaming();
						}
 
						LOG(DEBUG) << LABELS{"proto"} << "Stopped streaming (duration=" << getStreamingDuration(util::milliseconds).count() << " millisec)";
						//LOG(DEBUG) << LABELS{"proto"} << "Stopped streaming (duration=" << streamingStoppedAt.get(util::milliseconds) - streamingStartedAt.value().get(util::milliseconds)  << " millisec)";
					});

					streamingStartedAt.reset();
					playbackStartedAt.reset();
				}

				inline void streamChunk(Chunk& chunk)
				{
					// sending chunk to all SlimProto sessions
					for (auto& entry : commandSessions)
					{
						entry.second->streamChunk(chunk);
					}

					// increasing played frames counter
					streamingFrames += chunk.getFrames();

					LOG(DEBUG) << LABELS{"proto"} << "A chunk was distributed to the clients (total clients=" << streamingSessions.size() << ", frames=" << chunk.getFrames() << ")";
				}

			private:
				unsigned int                      streamingPort;
				EncoderBuilder                    encoderBuilder;
				std::optional<unsigned int>       gain;
				util::StateMachine<Event, State>  stateMachine;
				util::BigInteger                  nextID{0};
				SessionsMap<CommandSessionType>   commandSessions;
				SessionsMap<StreamingSessionType> streamingSessions;
				ts::optional<util::Timestamp>     streamingStartedAt{ts::nullopt};
				util::BigInteger                  streamingFrames{0};
				ts::optional<util::Timestamp>     playbackStartedAt{ts::nullopt};
				bool                              initializingStream{false};
		};
	}
}
