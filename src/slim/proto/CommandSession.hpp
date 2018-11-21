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
#include <conwrap2/Timer.hpp>
#include <cstddef>  // std::size_t
#include <functional>
#include <scope_guard.hpp>
#include <string>
#include <type_safe/optional.hpp>
#include <type_safe/optional_ref.hpp>
#include <unordered_map>
#include <vector>

#include "slim/ContainerBase.hpp"
#include "slim/Exception.hpp"
#include "slim/log/log.hpp"
#include "slim/proto/client/CommandDSCO.hpp"
#include "slim/proto/client/CommandHELO.hpp"
#include "slim/proto/client/CommandRESP.hpp"
#include "slim/proto/client/CommandSETD.hpp"
#include "slim/proto/client/CommandSTAT.hpp"
#include "slim/proto/server/CommandAUDE.hpp"
#include "slim/proto/server/CommandAUDG.hpp"
#include "slim/proto/server/CommandSETD.hpp"
#include "slim/proto/server/CommandSTRM.hpp"
#include "slim/proto/StreamingSession.hpp"
#include "slim/util/ArrayCache.hpp"
#include "slim/util/BigInteger.hpp"
#include "slim/util/ExpandableBuffer.hpp"
#include "slim/util/StateMachine.hpp"
#include "slim/util/Timestamp.hpp"


namespace slim
{
	namespace proto
	{
		namespace ts = type_safe;

		enum Event
		{
			HandshakeEvent,
			FlushedEvent,
			StartEvent,
			PlayEvent,
			StopEvent,
		};

		enum State
		{
			CreatedState,
			DrainingState,
			ReadyState,
			InitializingState,
			PlayingState,
		};

		template<typename ConnectionType>
		class CommandSession
		{
			using CommandHandlersMap = std::unordered_map<std::string, std::function<std::size_t(unsigned char*, std::size_t, util::Timestamp)>>;
			using EventHandlersMap   = std::unordered_map<std::string, std::function<void(client::CommandSTAT&, util::Timestamp)>>;

			public:
				CommandSession(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pr, std::reference_wrapper<ConnectionType> co, std::string id, unsigned int po, FormatSelection fo, std::optional<unsigned int> ga)
				: processorProxy{pr}
				, connection{co}
				, clientID{id}
				, streamingPort{po}
				, formatSelection{fo}
				, gain{ga}
				, commandHandlers
				{
					{"DSCO", [&](auto* buffer, auto size, auto timestamp) {return onDSCO(buffer, size);}},
					{"HELO", [&](auto* buffer, auto size, auto timestamp) {return onHELO(buffer, size);}},
					{"RESP", [&](auto* buffer, auto size, auto timestamp) {return onRESP(buffer, size);}},
					{"SETD", [&](auto* buffer, auto size, auto timestamp) {return onSETD(buffer, size);}},
					{"STAT", [&](auto* buffer, auto size, auto timestamp) {return onSTAT(buffer, size, timestamp);}},
				}
				, eventHandlers
				{
					{"STMc", [&](auto& commandSTAT, auto timestamp) {}},
					{"STMd", [&](auto& commandSTAT, auto timestamp) {}},
					{"STMf", [&](auto& commandSTAT, auto timestamp) {onSTMf(commandSTAT);}},
					{"STMl", [&](auto& commandSTAT, auto timestamp) {onSTMl(commandSTAT);}},
					{"STMo", [&](auto& commandSTAT, auto timestamp) {}},
					{"STMp", [&](auto& commandSTAT, auto timestamp) {}},
					{"STMr", [&](auto& commandSTAT, auto timestamp) {}},
					{"STMs", [&](auto& commandSTAT, auto timestamp) {onSTMs(commandSTAT);}},
					{"STMt", [&](auto& commandSTAT, auto timestamp) {onSTMt(commandSTAT, timestamp);}},
					{"STMu", [&](auto& commandSTAT, auto timestamp) {}},
				}
				, stateMachine
				{
					CreatedState,
					{   // transition table definition
						{HandshakeEvent, CreatedState,      DrainingState,     [&](auto event) {LOG(DEBUG) << "DRAINING";stateChangeToDraining();}, [&] {return true;}},
						{FlushedEvent,   DrainingState,     ReadyState,        [&](auto event) {LOG(DEBUG) << "READY";},                            [&] {return true;}},
						{FlushedEvent,   ReadyState,        ReadyState,        [&](auto event) {},                                                  [&] {return true;}},
						{FlushedEvent,   InitializingState, InitializingState, [&](auto event) {},                                                  [&] {return true;}},
						{FlushedEvent,   PlayingState,      PlayingState,      [&](auto event) {},                                                  [&] {return true;}},
						{StartEvent,     ReadyState,        InitializingState, [&](auto event) {LOG(DEBUG) << "INIT";stateChangeToInitializing();}, [&] {return isReadyToStream();}},
						{StartEvent,     InitializingState, InitializingState, [&](auto event) {},                                                  [&] {return true;}},
						{StartEvent,     PlayingState,      PlayingState,      [&](auto event) {},                                                  [&] {return true;}},
						{PlayEvent,      InitializingState, PlayingState,      [&](auto event) {LOG(DEBUG) << "PLAY";stateChangeToPlaying();},      [&] {return isReadyToPlay();}},
						{PlayEvent,      ReadyState,        ReadyState,        [&](auto event) {},                                                  [&] {return true;}},
						{PlayEvent,      PlayingState,      PlayingState,      [&](auto event) {},                                                  [&] {return true;}},
						{StopEvent,      PlayingState,      DrainingState,     [&](auto event) {LOG(DEBUG) << "DRAIN";stateChangeToDraining();},    [&] {return true;}},
						{StopEvent,      InitializingState, DrainingState,     [&](auto event) {LOG(DEBUG) << "DRAIN";stateChangeToDraining();},    [&] {return true;}},
						{StopEvent,      CreatedState,      CreatedState,      [&](auto event) {},                                                  [&] {return true;}},
						{StopEvent,      DrainingState,     DrainingState,     [&](auto event) {},                                                  [&] {return true;}},
						{StopEvent,      ReadyState,        ReadyState,        [&](auto event) {},                                                  [&] {return true;}},
					}
				}
				{
					LOG(DEBUG) << LABELS{"proto"} << "SlimProto session object was created (id=" << this << ")";
				}

				~CommandSession()
				{
					// canceling deferred operations if any
					ts::with(pingTimer, [&](auto& timer)
					{
						timer.cancel();
					});

					LOG(DEBUG) << LABELS{"proto"} << "SlimProto session object was deleted (id=" << this << ")";
				}

				CommandSession(const CommandSession&) = delete;             // non-copyable
				CommandSession& operator=(const CommandSession&) = delete;  // non-assignable
				CommandSession(CommandSession&& rhs) = delete;              // non-movable
				CommandSession& operator=(CommandSession&& rhs) = delete;   // non-movable-assignable

				inline auto getClientID()
				{
					return clientID;
				}

				inline auto isReadyToPlay()
				{
					return isReadyToStream() && streamingSession.has_value();
				}

				inline auto isReadyToStream()
				{
					return timeOffset.has_value();
				}

				inline void onRequest(unsigned char* buffer, std::size_t size, util::Timestamp timestamp)
				{
					// removing data from the buffer in case of an exception
					::util::scope_guard_failure onException = [&]
					{
						commandBuffer.size(0);
					};

					// adding data to the buffer
					commandBuffer.append(buffer, size);

					std::size_t keySize{4};
					std::size_t processedSize;
					do
					{
						processedSize = 0;

						// searching for a SlimProto Command based on a label in the buffer
						std::string label{(char*)commandBuffer.data(), commandBuffer.size() > keySize ? keySize : 0};
						auto        found{commandHandlers.find(label)};
						if (found != commandHandlers.end())
						{
							// if there is enough data to process this message
							auto data{commandBuffer.data()};
							auto size{commandBuffer.size()};
							if (Command<char>::isEnoughData(data, size))
							{
								// if this is not STAT command then reseting measuring flag
								// TODO: consider proper usage of Command label
								if (label.compare("STAT") != 0)
								{
									measuringLatency = false;
								}

								processedSize = (*found).second(data, size, timestamp);

								// removing processed data from the buffer
								// TODO: shrinking memory means moving data towards the beginning of the buffer; thing a better way
								commandBuffer.shrinkLeft(processedSize);
							}
						}
						else if (label.length() > 0)
						{
							LOG(WARNING) << LABELS{"proto"} << "Unsupported SlimProto command received (header='" << label << "')";

							// flushing command buffer
							commandBuffer.size(0);
						}
					} while (processedSize > 0);
				}

				inline void sendChunk(const Chunk& chunk)
				{
					// TODO: work in progress
					if (stateMachine.state == ReadyState && isReadyToStream())
					{
						// changing state to Initializing if needed
						stateMachine.processEvent(StartEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Send event - closing the connection";
							connection.get().stop();
						});
					}

					// TODO: implement accounting for amount of frames which were not sent out
					ts::with(streamingSession, [&](auto& streamingSession)
					{
						streamingSession.sendChunk(chunk);
					});

					if (chunk.isEndOfStream())
					{
						// changing state to Draining
						stateMachine.processEvent(StopEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Stop event - closing the connection";
							connection.get().stop();
						});
					}
				}

				inline void setStreamingSession(ts::optional_ref<StreamingSession<ConnectionType>> s)
				{
					streamingSession = s;
				}

				inline void startStreaming(unsigned int s, util::Timestamp t)
				{
					samplingRate       = s;
					streamingStartedAt = t;

					// TODO: parametrize
					bufferingThreshold       = std::chrono::milliseconds{500};
					bufferingThresholdFrames = bufferingThreshold.count() * samplingRate / 1000;
				}

				inline void stopStreaming()
				{
					streamingStartedAt.reset();
				}

			protected:
				inline auto calculateMean(std::vector<util::BigInteger>& samples)
				{
					util::BigInteger result{0};

					// sorting samples
					std::sort(samples.begin(), samples.end());

					// making sure there is enough sample
					if (samples.size() > 7)
					{
						// TODO: this is a temp version until BigInteger does not cause overflow
						result = samples[samples.size() >> 1];
					}

					return result;
				}

				inline auto onDSCO(unsigned char* buffer, std::size_t size)
				{
					std::size_t result{0};

					LOG(INFO) << LABELS{"proto"} << "DSCO command received";

					// deserializing DSCO command
					result = client::CommandDSCO{buffer, size}.getSize();

					return result;
				}

				inline void stateChangeToDraining()
				{
					// sending SlimProto Stop command will flush client's buffer which will initiate STAT/STMf message
					send(server::CommandSTRM{CommandSelection::Stop});

					// just to make sure previous HTTP session does not interfer with a new init routine
					streamingSession.reset();
				}

				inline void stateChangeToInitializing()
				{
					send(server::CommandSTRM{CommandSelection::Start, formatSelection, streamingPort, samplingRate, clientID});
				}

				inline void stateChangeToPlaying()
				{
					ts::with(streamingStartedAt, [&](auto& streamingStartedAt)
					{
						ts::with(timeOffset, [&](auto& timeOffset)
						{
							// TODO: work in progress
							auto playbackTime{streamingStartedAt.get(util::milliseconds) - timeOffset + 2000};
							send(server::CommandSTRM{CommandSelection::Unpause, static_cast<std::uint32_t>(playbackTime)});
						});
					});
				}

				inline auto onHELO(unsigned char* buffer, std::size_t size)
				{
					// deserializing HELO command
					auto h{client::CommandHELO{buffer, size}};
					auto result{h.getSize()};

					if (!commandHELO.has_value())
					{
						LOG(INFO) << LABELS{"proto"} << "HELO command received";

						commandHELO = h;
						send(server::CommandSETD{server::DeviceID::RequestName});
						send(server::CommandSETD{server::DeviceID::Squeezebox3});
						send(server::CommandAUDE{true, true});
						send(server::CommandAUDG{gain});

						// start sending ping commands after the 'handshake' commands
						pingTimer = ts::ref(processorProxy.processWithDelay([&]
						{
							// releasing timer so a new 'delayed' request may be issued
							pingTimer.reset();

							// creating a timestamp cache entry required to allocate a key
							ping(timestampCache.store(util::Timestamp::now()));
						}, std::chrono::seconds{1}));

						// changing state to Draining
						stateMachine.processEvent(HandshakeEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing HELO command - closing the connection";
							connection.get().stop();
						});
					}
					else
					{
						LOG(WARNING) << LABELS{"proto"} << "Multiple HELO command received - skipped";
					}

					return result;
				}

				inline auto onRESP(unsigned char* buffer, std::size_t size)
				{
					std::size_t result{0};

					LOG(INFO) << LABELS{"proto"} << "RESP command received";

					// deserializing RESP command
					result = client::CommandRESP{buffer, size}.getSize();

					return result;
				}

				inline auto onSETD(unsigned char* buffer, std::size_t size)
				{
					std::size_t result{0};

					LOG(INFO) << LABELS{"proto"} << "SETD command received";

					// deserializing SETD command
					result = client::CommandSETD{buffer, size}.getSize();

					return result;
				}

				inline auto onSTAT(unsigned char* buffer, std::size_t size, util::Timestamp receiveTimestamp)
				{
					client::CommandSTAT commandSTAT{buffer, size};
					std::size_t         result{commandSTAT.getSize()};

					auto event{commandSTAT.getEvent()};
					auto found{eventHandlers.find(event)};
					if (found != eventHandlers.end())
					{
						// if this is not STMt event then reseting measuring flag
						// TODO: consider proper usage of STAT event label
						if (event.compare("STMt") != 0)
						{
							LOG(DEBUG) << LABELS{"proto"} << event << " event received";
							measuringLatency = false;
						}

						// invoking STAT event handler
						(*found).second(commandSTAT, receiveTimestamp);
					}
					else
					{
						LOG(DEBUG) << LABELS{"proto"} << "Unsupported STAT event received: " << event;
					}

					return result;
				}

				inline void onSTMf(client::CommandSTAT& commandSTAT)
				{
					// changing state to Ready
					stateMachine.processEvent(FlushedEvent, [&](auto event, auto state)
					{
						LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Flush event - closing the connection";
						connection.get().stop();
					});
				}

				inline void onSTMl(client::CommandSTAT& commandSTAT)
				{
					auto bufferSize{commandSTAT.getBuffer()->streamBufferSize};
					auto fullness{commandSTAT.getBuffer()->streamBufferFullness};

					LOG(DEBUG) << LABELS{"proto"}
							<<  "bufferSize=" << bufferSize
							<< " fullness="   << fullness << " (" << 100 * fullness / bufferSize << "%)";

					// TODO: work in progress
					// changing state to Playing if needed
					stateMachine.processEvent(PlayEvent, [&](auto event, auto state)
					{
						LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Send event - closing the connection";
						connection.get().stop();
					});
				}

				inline void onSTMs(client::CommandSTAT& commandSTAT)
				{
					// TODO: work in progress
					auto bufferSize{commandSTAT.getBuffer()->streamBufferSize};
					auto fullness{commandSTAT.getBuffer()->streamBufferFullness};
					LOG(DEBUG) << LABELS{"proto"}
							<<  "bufferSize=" << bufferSize
							<< " fullness="   << fullness << " (" << 100 * fullness / bufferSize << "%)";

					LOG(DEBUG) << LABELS{"proto"} << "client played=" << commandSTAT.getBuffer()->elapsedMilliseconds;

					ts::with(streamingSession, [&](auto& streamingSession)
					{
						LOG(DEBUG) << LABELS{"proto"} << "server played=" << ((streamingSession.getFramesProvided() / samplingRate) * 1000);
					});
				}

				inline void onSTMt(client::CommandSTAT& commandSTAT, util::Timestamp receiveTimestamp)
				{
					auto timestampKey{commandSTAT.getBuffer()->serverTimestamp};
					auto sendTimestamp{ts::optional<util::Timestamp>{ts::nullopt}};

					// if request originated by a server
					if (timestampKey)
					{
						sendTimestamp = timestampCache.load(timestampKey);
					}

					// making sure it is a proper response from the client
					ts::with(sendTimestamp, [&](auto& sendTimestamp)
					{
						// if latency measurement was not interfered by other commands then repeat round trip once again
						if (measuringLatency)
						{
							// saving latency sample for further processing
							latencySamples.push_back((receiveTimestamp.get(util::microseconds) - sendTimestamp.get(util::microseconds)) / 2);
							timeOffsetSamples.push_back(sendTimestamp.get(util::milliseconds) - commandSTAT.getBuffer()->jiffies);

							// if there is enough samples to calculate latency
							if (timestampCache.size() >= timestampCache.capacity())
							{
								// calculating new latency value
								auto meanLatency{calculateMean(latencySamples)};
								auto meanTimeOffset{calculateMean(timeOffsetSamples)};

								// updating current latency value
								latency    = latency.value_or(meanLatency)       * 0.8 + meanLatency    * 0.2;
								timeOffset = timeOffset.value_or(meanTimeOffset) * 0.8 + meanTimeOffset * 0.2;
								LOG(DEBUG) << LABELS{"proto"} << "Client latency updated (client id=" << clientID << ", latency=" << latency.value() << " microsec)";

								// clearing the cache so it can be used for collecting new samples
								timestampCache.clear();
								latencySamples.clear();
								timeOffsetSamples.clear();

								// TODO: make it configurable
								// TODO: make it resistant to clients 'loosing' requests
								// submitting a new ping request if there is no other pending request
								if (!pingTimer.has_value())
								{
									pingTimer = ts::ref(processorProxy.processWithDelay([&]
									{
										// releasing timer so a new 'delayed' request may be issued
										pingTimer.reset();

										// creating a timestamp cache entry required to allocate a key
										ping(timestampCache.store(util::Timestamp::now()));
									}, std::chrono::seconds{5}));
								}
							}
							else
							{
								processorProxy.process([&]
								{
									// creating a timestamp cache entry required to allocate a key
									ping(timestampCache.store(util::Timestamp::now()));
								});
							}
						}
						else
						{
							LOG(DEBUG) << LABELS{"proto"} << "Latency probe was skipped";

							// capturing timestamp key by value
							processorProxy.process([&, timestampKey{timestampKey}]
							{
								ping(timestampKey);
							});
						}
					});
				}

				inline void ping(std::uint32_t timestampKey)
				{
					// creating a ping command
					auto command{server::CommandSTRM{CommandSelection::Time}};
					command.getBuffer()->data.replayGain = timestampKey;

					// capturing server timestamp as close as possible to the send operation
					auto timestamp{util::Timestamp::now()};

					// sending actual ping command
					send(command);

					// changing state to 'measuringLatency' which is used to track if there any interference while measuring latency
					measuringLatency = true;

					// storing the actual send timestamp in the cache
					timestampCache.update(timestampKey, timestamp);
				}

				template<typename CommandType>
				inline void send(CommandType&& command)
				{
					measuringLatency = false;

					// TODO: use local buffer and async API to prevent from interfering while measuring latency
					connection.get().write(command.getBuffer(), command.getSize());
				}

			private:
				conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy;
				std::reference_wrapper<ConnectionType>                   connection;
				std::string                                              clientID;
				unsigned int                                             streamingPort;
				FormatSelection                                          formatSelection;
				std::optional<unsigned int>                              gain;
				CommandHandlersMap                                       commandHandlers;
				EventHandlersMap                                         eventHandlers;
				util::StateMachine<Event, State>                         stateMachine;
				unsigned int                                             samplingRate{0};
				ts::optional_ref<StreamingSession<ConnectionType>>       streamingSession{ts::nullopt};
				util::ExpandableBuffer                                   commandBuffer{std::size_t{0}, std::size_t{2048}};
				ts::optional<client::CommandHELO>                        commandHELO{ts::nullopt};
				ts::optional_ref<conwrap2::Timer>                        pingTimer{ts::nullopt};
				util::ArrayCache<util::Timestamp, 10>                    timestampCache;
				// TODO: used std::chrono::microseconds instead of BigInteger
				ts::optional<util::BigInteger>                           latency{ts::nullopt};
				std::vector<util::BigInteger>                            latencySamples;
				ts::optional<util::BigInteger>                           timeOffset{ts::nullopt};
				std::vector<util::BigInteger>                            timeOffsetSamples;
				bool                                                     measuringLatency{false};
				ts::optional<util::Timestamp>                            streamingStartedAt;
				std::chrono::milliseconds                                bufferingThreshold{0};
				util::BigInteger                                         bufferingThresholdFrames{0};
		};
	}
}
