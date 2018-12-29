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
			using CommandSessionType   = CommandSession<ConnectionType, Streamer>;
			using StreamingSessionType = StreamingSession<ConnectionType>;

			enum Event
			{
				StartEvent,
				StreamEvent,
				PlayEvent,
				StopEvent,
			};

			enum State
			{
				ReadyState,
				InitializingState,
				BufferingState,
				PlayingState,
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
						{StartEvent,  ReadyState,        InitializingState, [&](auto event) {LOG(DEBUG) << "INIT";stateChangeToInitializing();}, [&] {return isReadyToInitialize();}},
						{StartEvent,  ReadyState,        ReadyState,        [&](auto event) {},                                                  [&] {return true;}},
						{StreamEvent, InitializingState, BufferingState,    [&](auto event) {LOG(DEBUG) << "BUFFER";},                           [&] {return isReadyToStream();}},
						{StreamEvent, BufferingState,    BufferingState,    [&](auto event) {},                                                  [&] {return true;}},
						{StreamEvent, PlayingState,      PlayingState,      [&](auto event) {},                                                  [&] {return true;}},
						{PlayEvent,   BufferingState,    PlayingState,      [&](auto event) {LOG(DEBUG) << "PLAY";stateChangeToPlaying();},      [&] {return isReadyToPlay();}},
						{PlayEvent,   PlayingState,      PlayingState,      [&](auto event) {},                                                  [&] {return true;}},
						{StopEvent,   InitializingState, ReadyState,        [&](auto event) {LOG(DEBUG) << "READY";stateChangeToReady();},       [&] {return true;}},
						{StopEvent,   BufferingState,    ReadyState,        [&](auto event) {LOG(DEBUG) << "READY";stateChangeToReady();},       [&] {return true;}},
						{StopEvent,   PlayingState,      ReadyState,        [&](auto event) {LOG(DEBUG) << "READY";stateChangeToReady();},       [&] {return true;}},
						{StopEvent,   ReadyState,        ReadyState,        [&](auto event) {},                                                  [&] {return true;}},
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
					auto result{ts::optional<bool>{ts::nullopt}};
					auto chunkSamplingRate{chunk.getSamplingRate()};

					// skipping chunks with sampling rate = 0
					if (!chunkSamplingRate)
					{
						LOG(WARNING) << LABELS{"proto"} << "Chunk was skipped due to invalid sampling rate (rate=0)";
						result = true;
					}

					// if this is the beginning of a stream then changing state to BufferingState
					if (!result.has_value() && !getSamplingRate().has_value())
					{
						// if transition went fine then assigning new sampling rate
						if (!result.has_value())
						{
							setSamplingRate(chunkSamplingRate);
							LOG(DEBUG) << LABELS{"proto"} << "Started streaming (rate=" << chunkSamplingRate << ")";
						}

						stateMachine.processEvent(StartEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Start event - skipping chunk";
							result = true;
						});
					}

					// if streaming is ongoing (the most common case)
					if (!result.has_value() && chunkSamplingRate == getSamplingRate().value_or(0))
					{
						// if initialization is ongoing
						// TODO: think of a better way to identify states
						if (stateMachine.state == InitializingState)
						{
							stateMachine.processEvent(StreamEvent, [&](auto event, auto state)
							{
								LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Stream event - skipping chunk";
								result = true;
							});
						}

						// if buffering is still ongoing then ckecking if it's completed
						if (stateMachine.state == BufferingState)
						{
							stateMachine.processEvent(PlayEvent, [&](auto event, auto state)
							{
								LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Play event - skipping chunk";
								result = true;
							});
						}

						// do not distribute a chunk until initializtion is complete
						if (stateMachine.state != InitializingState)
						{
							streamChunk(chunk);
						}
					}

					// TODO: consider DrainingState
					// if sampling rate is changing in the middle of the stream then a new stream must be initialized
					if (!result.has_value() && chunkSamplingRate != getSamplingRate().value_or(chunkSamplingRate))
					{
						LOG(WARNING) << LABELS{"proto"} << "Sampling rate has changed in the middle of a stream (current rate=" << getSamplingRate().value_or(0) << "; new rate=" << chunkSamplingRate << ")";

						auto transitionSucceeded{true};
						stateMachine.processEvent(StopEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Stop event - skipping chunk";
							transitionSucceeded = false;
						});

						if (transitionSucceeded)
						{
							result = false;
						}
					}

					auto chunkWillBeReprocessed = result.map([](auto& result)
					{
						return result;
					}).value_or(false);

					// if chunk will not be reprocessed and it is the end of a stream then changing state to ReadyState
					if (!chunkWillBeReprocessed && chunk.isEndOfStream())
					{
						auto transitionSucceeded{true};
						stateMachine.processEvent(StopEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Stop event - skipping chunk";
							transitionSucceeded = false;
							result              = false;
						});

						if (transitionSucceeded)
						{
							result = true;
						}
					}

					return result.value_or(true);
				}

				void onHTTPClose(ConnectionType& connection)
				{
					LOG(DEBUG) << LABELS{"proto"} << "HTTP session close callback (connection=" << &connection << ")";

					if (auto found{streamingSessions.find(&connection)}; found != streamingSessions.end())
					{
						auto sessionPtr = std::move((*found).second);
						streamingSessions.erase(found);
						LOG(DEBUG) << LABELS{"proto"} << "HTTP session was removed (id=" << sessionPtr.get()
						                              << ", total sessions=" << streamingSessions.size() << ")";

						// if there is a relevant SlimProto session then reset the reference
						auto clientID{sessionPtr->getClientID()};
						if (auto commandSession{findSessionByID(commandSessions, clientID)}; commandSession.has_value())
						{
							commandSession.value()->setStreamingSession(ts::nullopt);
						}
						else
						{
							LOG(WARNING) << LABELS{"proto"} << "Could not find SlimProto session by client ID (clientID=" << clientID << ")";
						}
					}
					else
					{
						LOG(WARNING) << LABELS{"proto"} << "Could not find HTTP session by provided connection";
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

							// saving HTTP session reference in the relevant SlimProto session
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
					LOG(DEBUG) << LABELS{"proto"} << "HTTP session open callback (connection=" << &connection << ")";
				}

				void onSlimProtoClose(ConnectionType& connection)
				{
					LOG(DEBUG) << LABELS{"proto"} << "SlimProto close callback (connection=" << &connection << ")";

					if (auto foundSession{commandSessions.find(&connection)}; foundSession != commandSessions.end())
					{
						// removing session from the owning container
						auto sessionPtr = std::move((*foundSession).second);
						commandSessions.erase(foundSession);

						LOG(DEBUG) << LABELS{"proto"} << "SlimProto session was removed (id=" << sessionPtr.get()
						                              << ", total sessions=" << commandSessions.size() << ")";
					}
					else
					{
						LOG(WARNING) << LABELS{"proto"} << "Could not find SlimProto session by provided connection";
					}
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
					LOG(DEBUG) << LABELS{"proto"} << "SlimProto session open callback (connection=" << &connection << ")";

					// using regular counter for session ID's instead of MAC's; it allows running multiple players on one host
					std::stringstream ss;
					ss << (++nextID);

					// creating command session object
					auto commandSessionPtr{std::make_unique<CommandSessionType>(std::ref<ConnectionType>(connection), std::ref<Streamer>(*this), ss.str(), streamingPort, encoderBuilder.getFormat(), gain)};

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
					auto maxLatency{std::chrono::microseconds{0}};

					for (auto& entry : commandSessions)
					{
						auto latency{entry.second->getLatency()};
						ts::with(latency, [&](auto& latency)
						{
							if (maxLatency < latency)
							{
								maxLatency = latency;
							}
						});
					}

					// adding extra 10 milliseconds required for sending out command to all the clients
					return util::Timestamp::now() + std::chrono::milliseconds{10} + maxLatency;
				}

				template<typename SessionType>
				auto findSessionByID(SessionsMap<SessionType>& sessions,  std::string clientID)
				{
					auto result{ts::optional<SessionType*>{ts::nullopt}};
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

				inline auto isReadyToInitialize()
				{
					return getSamplingRate().has_value();
				}

				inline auto isReadyToPlay()
				{
					// TODO: introduce timeout threshold
					return 0 == std::count_if(commandSessions.begin(), commandSessions.end(), [&](auto& entry)
					{
						return !entry.second->isReadyToPlay();
					});
				}

				inline auto isReadyToStream()
				{
					auto result{false};
					auto now = util::Timestamp::now();
					// TODO: deferring time-out should be configurable
					auto waitThresholdReached{std::chrono::milliseconds{500} < (now - initializationStartedAt.value_or(now))};
					auto notReadyToStreamTotal{std::count_if(commandSessions.begin(), commandSessions.end(), [&](auto& entry)
					{
						return !entry.second->isReadyToStream();
					})};

					// if deferring time-out has expired
					if (waitThresholdReached)
					{
						result = true;

						if (notReadyToStreamTotal)
						{
							LOG(WARNING) << LABELS{"proto"} << "Could not defer chunk processing due to reached threshold";
						}
					}
					else if (!notReadyToStreamTotal)
					{
						// all SlimProto sessions are ready to stream
						result = true;
					}

					return result;
				}

				inline void stateChangeToInitializing()
				{
					// initialization start time is required for calculating defer time-out
					initializationStartedAt = util::Timestamp::now();

					ts::with(getSamplingRate(), [&](auto samplingRate)
					{
						LOG(DEBUG) << LABELS{"proto"} << "total sessions=" << commandSessions.size();
						for (auto& entry : commandSessions)
						{
							entry.second->prepare(samplingRate);
						}
					});
				}

				inline void stateChangeToPlaying()
				{
					// capturing start playback time point
					auto timestamp{calculatePlaybackTime()};
					playbackStartedAt = timestamp;

					for (auto& entry : commandSessions)
					{
						entry.second->startPlayback(timestamp);
					}
				}

				inline void stateChangeToReady()
				{
					if (getSamplingRate().has_value())
					{
						LOG(DEBUG) << LABELS{"proto"} << "Stopped streaming (duration=" << getStreamingDuration(util::milliseconds).count() << " millisec)";
					}

					// resetting state
					initializationStartedAt.reset();
					playbackStartedAt.reset();
					setSamplingRate(ts::nullopt);
					streamingFrames = 0;
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
				ts::optional<util::Timestamp>     initializationStartedAt{ts::nullopt};
				ts::optional<util::Timestamp>     playbackStartedAt{ts::nullopt};
				util::BigInteger                  streamingFrames{0};
		};
	}
}
