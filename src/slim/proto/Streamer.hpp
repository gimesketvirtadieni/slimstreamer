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
				CreateEvent,
				ReadyEvent,
				PrepareEvent,
				BufferEvent,
				PlayEvent,
				DrainEvent,
			};

			enum State
			{
				CreatedState,
				ReadyState,
				PreparingState,
				BufferingState,
				PlayingState,
				DrainingState,
			};

			public:
				Streamer(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pp, unsigned int sp, EncoderBuilder eb, ts::optional<unsigned int> ga)
				: Consumer{pp}
				, streamingPort{sp}
				, encoderBuilder{eb}
				, gain{ga}
				, stateMachine
				{
					CreatedState,  // initial state
					{   // transition table definition
						{ReadyEvent,   CreatedState,   ReadyState,     [&](auto event) {},                          [&] {return true;}},
						{ReadyEvent,   DrainingState,  ReadyState,     [&](auto event) {},                          [&] {return !isDraining();}},
						{ReadyEvent,   ReadyState,     ReadyState,     [&](auto event) {},                          [&] {return true;}},
						{PrepareEvent, ReadyState,     PreparingState, [&](auto event) {stateChangeToPreparing();}, [&] {return true;}},
						{PrepareEvent, PreparingState, PreparingState, [&](auto event) {},                          [&] {return true;}},
						{BufferEvent,  PreparingState, BufferingState, [&](auto event) {stateChangeToBuffering();}, [&] {return isReadyToBuffer();}},
						{BufferEvent,  BufferingState, BufferingState, [&](auto event) {},                          [&] {return true;}},
						{BufferEvent,  PlayingState,   PlayingState,   [&](auto event) {},                          [&] {return true;}},
						{PlayEvent,    BufferingState, PlayingState,   [&](auto event) {stateChangeToPlaying();},   [&] {return isReadyToPlay();}},
						{PlayEvent,    PlayingState,   PlayingState,   [&](auto event) {},                          [&] {return true;}},
						{DrainEvent,   PreparingState, DrainingState,  [&](auto event) {},                          [&] {return true;}},
						{DrainEvent,   BufferingState, DrainingState,  [&](auto event) {},                          [&] {return true;}},
						{DrainEvent,   PlayingState,   DrainingState,  [&](auto event) {},                          [&] {return true;}},
						{DrainEvent,   DrainingState,  DrainingState,  [&](auto event) {},                          [&] {return true;}},
						{DrainEvent,   ReadyState,     ReadyState,     [&](auto event) {},                          [&] {return true;}},
						{CreateEvent,  CreatedState,   CreatedState,   [&](auto event) {},                          [&] {return true;}},
						{CreateEvent,  ReadyState,     CreatedState,   [&](auto event) {},                          [&] {return true;}},
						{CreateEvent,  PreparingState, CreatedState,   [&](auto event) {/*???*/},                   [&] {return true;}},
						{CreateEvent,  BufferingState, CreatedState,   [&](auto event) {/*???*/},                   [&] {return true;}},
						{CreateEvent,  PlayingState,   CreatedState,   [&](auto event) {/*???*/},                   [&] {return true;}},
						{CreateEvent,  DrainingState,  CreatedState,   [&](auto event) {/*???*/},                   [&] {return true;}},
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
				Streamer& operator=(Streamer&& rhs) = delete;   // non-movable-assignable

				virtual bool consumeChunk(Chunk& chunk) override
				{
					auto result{false};
					auto chunkSamplingRate{chunk.getSamplingRate()};

					if (stateMachine.state == ReadyState)
					{
						if (chunkSamplingRate)
						{
							LOG(DEBUG) << LABELS{"proto"} << "Started streaming (rate=" << chunkSamplingRate << ")";
							samplingRate = chunkSamplingRate;

							// trying to change state to Preparing
							stateMachine.processEvent(PrepareEvent, [&](auto event, auto state)
							{
								LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Start event - skipping chunk";
								result = true;
							});

							// chunk will be reprocessed after PreparingState is over
							result = false;
						}
						else
						{
							LOG(WARNING) << LABELS{"proto"} << "Chunk was skipped due to invalid sampling rate (rate=0)";
							result = true;
						}
					}

					if (stateMachine.state == PreparingState)
					{
						// trying to change state to Buffering
						result = stateMachine.processEvent(BufferEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Stream event - skipping chunk";
							result = true;
						});
					}

					if (stateMachine.state == BufferingState || stateMachine.state == PlayingState)
					{
						if (stateMachine.state == BufferingState)
						{
							// buffering is still ongoing so trying to transition to Playing state
							stateMachine.processEvent(PlayEvent, [&](auto event, auto state)
							{
								LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Play event - skipping chunk";
								result = true;
							});
						}

						// if sampling rate does not change then just distributing a chunk
						if (samplingRate == chunkSamplingRate)
						{
							streamChunk(chunk);
							result = true;
						}

						if (samplingRate != chunkSamplingRate || chunk.isEndOfStream())
						{
							// changing state to Draining
							stateMachine.processEvent(DrainEvent, [&](auto event, auto state)
							{
								LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Stop event - skipping chunk";
								result = true;
							});

							// keep reprocessing until Ready state is reached
							result = false;
						}
					}

					if (stateMachine.state == DrainingState)
					{
						// trying to transition to Ready state which will succeed only when all SlimProto sessions are in Ready state
						result = stateMachine.processEvent(ReadyEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Stop event - skipping chunk";
							result = true;
						});
						if (auto duration{getStreamingDuration(util::milliseconds).count()}; result && duration)
						{
							LOG(DEBUG) << LABELS{"proto"} << "Stopped streaming (duration=" << duration << " millisec)";
						}
					}

					return result;
				}

				template<typename RatioType>
				inline auto getPlaybackTime(const RatioType& ratio) const
				{
					return playbackStartedAt + calculateDuration(streamingFrames - bufferedFrames, ratio);
				}

				inline auto getSamplingRate() const
				{
					return samplingRate;
				}

				template<typename RatioType>
				inline auto getStreamingDuration(const RatioType& ratio) const
				{
					return calculateDuration(streamingFrames, ratio);
				}

				inline auto isDraining()
				{
					return 0 < std::count_if(commandSessions.begin(), commandSessions.end(), [&](auto& entry)
					{
						return entry.second->isDraining();
					});
				}

				inline auto isPlaying()
				{
					return stateMachine.state == PlayingState;
				}

				virtual bool isRunning() override
				{
					return stateMachine.state != CreatedState;
				}

				void onHTTPClose(ConnectionType& connection)
				{
					LOG(DEBUG) << LABELS{"proto"} << "HTTP session close callback (connection=" << &connection << ")";

					if (auto found{streamingSessions.find(&connection)}; found != streamingSessions.end())
					{
						// if there is a relevant SlimProto session then reset the reference
						auto clientID{(*found).second->getClientID()};
						if (auto commandSession{findSessionByID(commandSessions, clientID)}; commandSession.has_value())
						{
							commandSession.value()->setStreamingSession(ts::nullopt);
						}

						(*found).second->stop([&, sessionPtr = (*found).second.get()]
						{
							// submitting a new handler is required as it deletes session object which runs this callback
							getProcessorProxy().process([&, sessionPtr = sessionPtr]
							{
								// this loop will be replaced with std::erase_if once it is implemented by libstdc++
								for (auto i = streamingSessions.begin(), last = streamingSessions.end(); i != last;)
								{
									if (sessionPtr == (*i).second.get())
									{
										i = streamingSessions.erase(i);

										LOG(DEBUG) << LABELS{"proto"}
											<< "HTTP session was removed (id=" << sessionPtr
											<< ", total sessions=" << streamingSessions.size()
											<< ")";
									}
									else
									{
										i++;
									}
								}
							});
						});
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

						// setting encoder sampling rate
						encoderBuilder.setSamplingRate(samplingRate);

						// creating streaming session object
						auto streamingSessionPtr{std::make_unique<StreamingSessionType>(getProcessorProxy(), std::ref<ConnectionType>(connection), clientID.value(), encoderBuilder)};
						streamingSessionPtr->start();

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
					auto commandSessionPtr{std::make_unique<CommandSessionType>(getProcessorProxy(), std::ref<ConnectionType>(connection), std::ref<Streamer>(*this), ss.str(), streamingPort, encoderBuilder.getFormat(), gain)};

					// saving command session in the map
					addSession(commandSessions, connection, std::move(commandSessionPtr));
				}

				virtual void start() override
				{
					// changing state to Ready
					stateMachine.processEvent(ReadyEvent, [&](auto event, auto state)
					{
						LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Ready event";
					});
				}

				virtual void stop(bool gracefully = true) override
				{
					// changing state to Created
					stateMachine.processEvent(CreateEvent, [&](auto event, auto state)
					{
						LOG(WARNING) << LABELS{"proto"} << "Invalid Streamer state while processing Create event";
					});
				}

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

				template<typename RatioType>
				inline auto calculateDuration(const util::BigInteger& frames, const RatioType& ratio) const
 				{
					auto result{std::chrono::duration<long long, RatioType>{0}};

					if (samplingRate)
					{
						result = std::chrono::duration<long long, RatioType>{frames * ratio.den / samplingRate};
					}

					return result;
 				}

				inline auto calculatePlaybackTime()
				{
					auto maxLatency{std::chrono::microseconds{0}};

					for (auto& entry : commandSessions)
					{
						ts::with(entry.second->getLatency(), [&](const auto& latency)
						{
							if (maxLatency < latency)
							{
								maxLatency = latency;
							}
						});
					}

					// TODO: parameterize
					// adding extra 10 milliseconds required for sending out command to all the clients
					return util::Timestamp::now() + maxLatency + std::chrono::milliseconds{10};
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

				inline auto isReadyToBuffer()
				{
					auto result{false};

					// TODO: deferring time-out should be configurable
					auto waitThresholdReached{std::chrono::milliseconds{2000} < (util::Timestamp::now() - preparingStartedAt)};
					auto notReadyToStreamTotal{std::count_if(commandSessions.begin(), commandSessions.end(), [&](auto& entry)
					{
						return !entry.second->isReadyToBuffer();
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

				inline auto isReadyToPlay()
				{
					auto result{false};

					// TODO: min buffering period should be configurable
					if (auto minThresholdReached{std::chrono::milliseconds{2000} < getStreamingDuration(util::milliseconds)}; minThresholdReached)
					{
						// TODO: introduce max timeout threshold
						result = (0 == std::count_if(commandSessions.begin(), commandSessions.end(), [&](auto& entry)
						{
							return !entry.second->isReadyToPlay();
						}));
					}

					return result;
				}

				inline void stateChangeToBuffering()
				{
					// buffering start time is required for calculating min buffering time
					bufferingStartedAt = util::Timestamp::now();
				}

				inline void stateChangeToPlaying()
				{
					// capturing start playback time point and how many frames were buffered
					playbackStartedAt = calculatePlaybackTime();
                    bufferedFrames    = streamingFrames;
				}

				inline void stateChangeToPreparing()
				{
					// preparing start time is required for calculating defer time-out
					preparingStartedAt = util::Timestamp::now();
					streamingFrames    = 0;
                    bufferedFrames     = 0;

					for (auto& entry : commandSessions)
					{
						entry.second->prepare(samplingRate);
					}
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

					LOG(DEBUG) << LABELS{"proto"} << "A chunk was distributed to the clients (total clients=" << commandSessions.size() << ", frames=" << chunk.getFrames() << ")";
				}

			private:
				unsigned int                      streamingPort;
				EncoderBuilder                    encoderBuilder;
				ts::optional<unsigned int>        gain;
				util::StateMachine<Event, State>  stateMachine;
				util::BigInteger                  nextID{0};
				SessionsMap<CommandSessionType>   commandSessions;
				SessionsMap<StreamingSessionType> streamingSessions;
				unsigned int                      samplingRate{0};
				util::Timestamp                   preparingStartedAt{util::Timestamp::now()};
				util::Timestamp                   bufferingStartedAt{util::Timestamp::now()};
				util::Timestamp                   playbackStartedAt{util::Timestamp::now()};
				util::BigInteger                  streamingFrames{0};
				util::BigInteger                  bufferedFrames{0};
		};
	}
}
