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
#include <cstddef>  // std::size_t, std::uint8_t
#include <functional>
#include <memory>
#include <scope_guard.hpp>
#include <string>
#include <sstream>
#include <type_safe/optional.hpp>
#include <type_safe/optional_ref.hpp>
#include <unordered_map>
#include <vector>

#include "slim/Chunk.hpp"
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
#include "slim/util/Duration.hpp"
#include "slim/util/buffer/Helper.hpp"
#include "slim/util/buffer/Ring.hpp"
#include "slim/util/StateMachine.hpp"
#include "slim/util/Timestamp.hpp"


namespace slim
{
	namespace proto
	{
		namespace ts = type_safe;

		template<typename ConnectionType, typename StreamerType>
		class CommandSession
		{
			protected:
				using CommandHandlersMap = std::unordered_map<std::string, std::function<std::size_t(util::Timestamp)>>;
				using EventHandlersMap   = std::unordered_map<std::string, std::function<void(client::CommandSTAT&, util::Timestamp)>>;

				enum Event
				{
					StartEvent,
					HandshakeEvent,
					PrepareEvent,
					BufferEvent,
					PlayEvent,
					DrainEvent,
					FlushedEvent,
					StopEvent,
				};

				enum State
				{
					StartedState,
					PreparingState,
					BufferingState,
					PlayingState,
					DrainingState,
					StoppedState,
				};

				struct ProbeValues
				{
					util::Timestamp sendTimestamp;
					util::Duration  latency;
					util::Timestamp clientTimestamp;
					util::Duration  clientDuration;
				};

			public:
				CommandSession(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pp, std::reference_wrapper<ConnectionType> co, std::reference_wrapper<StreamerType> st, std::string id, unsigned int po, FormatSelection fo, ts::optional<unsigned int> ga)
				: processorProxy{pp}
				, connection{co}
				, streamer{st}
				, clientID{id}
				, streamingPort{po}
				, formatSelection{fo}
				, gain{ga}
				, commandHandlers
				{
					{"DSCO", [&](auto timestamp) {return onDSCO();}},
					{"HELO", [&](auto timestamp) {return onHELO();}},
					{"RESP", [&](auto timestamp) {return onRESP();}},
					{"SETD", [&](auto timestamp) {return onSETD();}},
					{"STAT", [&](auto timestamp) {return onSTAT(timestamp);}},
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
					{"STMu", [&](auto& commandSTAT, auto timestamp) {onSTMu(commandSTAT);}},
				}
				, stateMachine
				{
					StoppedState,  // initial state
					{   // transition table definition
						{StartEvent,     StartedState,   StartedState,   [&](auto event) {},                          [&] {return true;}},
						{StartEvent,     PreparingState, PreparingState, [&](auto event) {},                          [&] {return true;}},
						{StartEvent,     BufferingState, BufferingState, [&](auto event) {},                          [&] {return true;}},
						{StartEvent,     PlayingState,   PlayingState,   [&](auto event) {},                          [&] {return true;}},
						{StartEvent,     DrainingState,  DrainingState,  [&](auto event) {},                          [&] {return true;}},
						{StartEvent,     StoppedState,   StartedState,   [&](auto event) {},                          [&] {return true;}},
						{HandshakeEvent, StartedState,   DrainingState,  [&](auto event) {},                          [&] {return true;}},
						{PrepareEvent,   StartedState,   PreparingState, [&](auto event) {stateChangeToPreparing();}, [&] {return isReadyToPrepare();}},
						{PrepareEvent,   PreparingState, PreparingState, [&](auto event) {},                          [&] {return true;}},
						{PrepareEvent,   BufferingState, BufferingState, [&](auto event) {},                          [&] {return true;}},
						{PrepareEvent,   PlayingState,   PlayingState,   [&](auto event) {},                          [&] {return true;}},
						{BufferEvent,    PreparingState, BufferingState, [&](auto event) {},                          [&] {return isReadyToBuffer();}},
						{BufferEvent,    BufferingState, BufferingState, [&](auto event) {},                          [&] {return true;}},
						{BufferEvent,    PlayingState,   PlayingState,   [&](auto event) {},                          [&] {return true;}},
						{PlayEvent,      BufferingState, PlayingState,   [&](auto event) {stateChangeToPlaying();},   [&] {return isReadyToPlay();}},
						{PlayEvent,      PlayingState,   PlayingState,   [&](auto event) {},                          [&] {return true;}},
						{DrainEvent,     PreparingState, DrainingState,  [&](auto event) {},                          [&] {return true;}},
						{DrainEvent,     BufferingState, DrainingState,  [&](auto event) {},                          [&] {return true;}},
						{DrainEvent,     PlayingState,   DrainingState,  [&](auto event) {},                          [&] {return true;}},
						{DrainEvent,     DrainingState,  DrainingState,  [&](auto event) {},                          [&] {return true;}},
						{DrainEvent,     StartedState,   StartedState,   [&](auto event) {},                          [&] {return true;}},
						{FlushedEvent,   StartedState,   StartedState,   [&](auto event) {},                          [&] {return true;}},
						{FlushedEvent,   PreparingState, PreparingState, [&](auto event) {},                          [&] {return true;}},
						{FlushedEvent,   PlayingState,   PlayingState,   [&](auto event) {},                          [&] {return true;}},
						{FlushedEvent,   DrainingState,  StartedState,   [&](auto event) {},                          [&] {return true;}},
						{StopEvent,      StoppedState,   StoppedState,   [&](auto event) {},                          [&] {return true;}},
						{StopEvent,      StartedState,   StoppedState,   [&](auto event) {stateChangeToStopped();},   [&] {return true;}},
						{StopEvent,      PreparingState, StoppedState,   [&](auto event) {stateChangeToStopped();},   [&] {return true;}},
						{StopEvent,      BufferingState, StoppedState,   [&](auto event) {stateChangeToStopped();},   [&] {return true;}},
						{StopEvent,      PlayingState,   StoppedState,   [&](auto event) {stateChangeToStopped();},   [&] {return true;}},
						{StopEvent,      DrainingState,  StoppedState,   [&](auto event) {stateChangeToStopped();},   [&] {return true;}},
					}
				}
				{
					LOG(DEBUG) << LABELS{"proto"} << "SlimProto session object was created (id=" << this << ")";
				}

				~CommandSession()
				{
					// canceling deferred operation
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

				inline bool consumeChunk(const Chunk& chunk)
				{
					auto result = true;

					if (stateMachine.state == StartedState)
					{
						// calling prepare() here is required for cases when SlimProto session is created while streaming
						prepare(streamer.get().getSamplingRate());
					}

					if (stateMachine.state == PreparingState)
					{
						// 'trying' to change state to Buffering
						stateMachine.processEvent(BufferEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Buffer event - closing the connection";
							connection.get().stop();
						});
					}

					if (stateMachine.state == BufferingState || stateMachine.state == PlayingState)
					{
						if (stateMachine.state == BufferingState && streamer.get().isPlaying())
						{
							// changing state to Playing
							stateMachine.processEvent(PlayEvent, [&](auto event, auto state)
							{
								LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Play event - closing the connection";
								connection.get().stop();
							});
						}

						// TODO: implement accounting for amount of frames which were not sent out
						ts::with(streamingSession, [&](auto& streamingSession)
						{
							result = streamingSession.consumeChunk(chunk);
						});

						// if chunk was consumed and it contains end-of-stream signal then changing state to Draining
						if (result)
						{
							if (chunk.endOfStream)
							{
								stateMachine.processEvent(DrainEvent, [&](auto event, auto state)
								{
									LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Drain event - closing the connection";
									connection.get().stop();
								});
							}
							lastChunkTimestamp      = chunk.timestamp;
							lastChunkCapturedFrames = chunk.capturedFrames;
						}
					}

					return result;
				}

				inline auto getClientID()
				{
					return clientID;
				}

				inline auto getConnection()
				{
					return connection;
				}

				inline auto getLatency()
				{
					return latency;
				}

				inline auto isDraining()
				{
					return stateMachine.state == DrainingState;
				}

				inline auto isReadyToBuffer()
				{
					return isReadyToPrepare() && streamingSession.has_value();
				}

				inline auto isReadyToPrepare()
				{
					return samplingRate != 0;
				}

				inline auto isReadyToPlay()
				{
					return isReadyToPrepare() && isReadyToBuffer() && timeOffset.has_value() && clientBufferIsReady;
				}

				inline bool isRunning()
				{
					return stateMachine.state != StoppedState;
				}

				inline void onRequest(unsigned char* buffer, std::size_t size, util::Timestamp timestamp)
				{
					// removing data from the buffer in case of an exception
					::util::scope_guard_failure onException = [&]
					{
						commandRingBuffer.clear();
					};

					// adding data to the buffer
					for (std::size_t i{0}; i < size; i++)
					{
						commandRingBuffer.push(buffer[i]);
					}
/*
					std::stringstream ss;
					ss << "Command buffer content:" << std::endl;
					util::buffer::writeToStream(commandRingBuffer, commandRingBuffer.getSize(), ss);
					LOG(INFO) << LABELS{"proto"} << ss.str();
*/
					// keep processing until there is anything to process in the buffer
					std::size_t keySize{4};
					std::size_t processedSize;
					do
					{
						processedSize = 0;
						auto label{ts::optional<std::string>{ts::nullopt}};

						if (keySize <= commandRingBuffer.getSize())
						{
							char labelCharacters[keySize] = {(char)commandRingBuffer[0], (char)commandRingBuffer[1], (char)commandRingBuffer[2], (char)commandRingBuffer[3]};
							label = std::string{labelCharacters, keySize};
						}

						auto found{commandHandlers.end()};
						ts::with(label, [&](auto& label)
						{
							// searching for a command handler based on provided label
							found = commandHandlers.find(label);

							// label is present in the buffer but no command handler was found
							if (found == commandHandlers.end())
							{
								LOG(WARNING) << LABELS{"proto"} << "Unsupported SlimProto command received, skipping one character (header='" << label << "')";

								commandRingBuffer.pop();
							}
						});

						if (found != commandHandlers.end())
						{
							// if there is enough data to process this message
							if (InboundCommand<char>::isEnoughData(commandRingBuffer))
							{
								// if this is not STAT command then reseting measuring flag
								// TODO: consider proper usage of Command label
								if (label.value().compare("STAT") != 0)
								{
									measuringLatency = false;
								}

								// processing data
								processedSize = (*found).second(timestamp);

								// removing processed data from the buffer
								for (auto i{std::size_t{0}}; i < processedSize; i++)
								{
									commandRingBuffer.pop();
								}
							}
						}
					} while (processedSize > 0);
				}

				inline auto prepare(unsigned int s)
				{
					auto temp{samplingRate};
					samplingRate = s;

					// 'trying' to change state to Preparing
					if (!stateMachine.processEvent(PrepareEvent, [&](auto event, auto state)
					{
						LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Prepare event";
					}))
					{
						// restoring sampling rate if transition failed
						samplingRate = temp;
					}
				}

				inline void setStreamingSession(ts::optional_ref<StreamingSession<ConnectionType, StreamerType>> s)
				{
					streamingSession = s;
				}

				inline void start()
				{
					// changing state to Started if needed
					stateMachine.processEvent(StartEvent, [&](auto event, auto state)
					{
						LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Start event - closing the connection";
						connection.get().stop();
					});
				}

				template <typename CallbackType>
				inline void stop(CallbackType callback)
				{
					if (isRunning())
					{
						// sending SlimProto Stop command if session is not idle
						if (stateMachine.state != StartedState)
						{
							send(server::CommandSTRM{CommandSelection::Stop});
						}

						// transition to Stopped state
						stateMachine.processEvent(StopEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Stop event";
						});
					}

					callback();
				}

			protected:
				inline auto getAccurateDurationIndex() const
				{
					auto result{ts::optional<std::size_t>{ts::nullopt}};

					for (auto i{std::size_t{probes.capacity()}}; 2 <= i && !result.has_value(); i--)
					{
						auto& probe1{probes[i - 1]};
						auto& probe2{probes[i - 2]};

						if (probe1.clientDuration.count() && probe2.clientDuration.count() && probe1.clientDuration != probe2.clientDuration)
						{
							result = i - 1;
						}
					}

					return result;
				}

				inline auto getLatencyMeanProbe()
				{
					auto result{ProbeValues{}};
					auto samples{std::vector<ProbeValues>{}};

					// skipping the first 3 elements while calculating 'mean probe' due to TCP 'slow start' (https://en.wikipedia.org/wiki/TCP_congestion_control)
					for (auto i{std::size_t{3}}; i < probes.capacity(); i++)
					{
						if (probes[i].latency != probes[i].latency.zero())
						{
							samples.push_back(probes[i]);
						}
					}

					// sorting samples if needed
					if (3 <= samples.size())
					{
						std::sort(samples.begin(), samples.end(), [](auto& lhs, auto& rhs)
						{
							return lhs.latency < rhs.latency;
						});
					}

					// picking mean value
					if (!samples.empty())
					{
						result = samples[samples.size() >> 1];
					}

					return result;
				}

				inline auto onDSCO()
				{
					std::size_t result{0};

					LOG(INFO) << LABELS{"proto"} << "DSCO command received";

					// deserializing DSCO command
					result = client::CommandDSCO{commandRingBuffer}.getSize();

					return result;
				}

				inline auto onHELO()
				{
					// deserializing HELO command
					auto h{client::CommandHELO{commandRingBuffer}};
					auto result{h.getSize()};

					if (!commandHELO.has_value())
					{
						LOG(INFO) << LABELS{"proto"} << "HELO command received";

						commandHELO = std::move(h);

						send(server::CommandSETD{server::DeviceID::RequestName});
						send(server::CommandSETD{server::DeviceID::Squeezebox3});
						send(server::CommandAUDE{true, true});
						send(server::CommandAUDG{gain});

						// start sending ping commands after the 'handshake' commands
						if (!pingTimer.has_value())
						{
							pingTimer = ts::ref(processorProxy.processWithDelay([&]
							{
								// releasing timer so a new 'delayed' request may be issued
								pingTimer.reset();

								// allocating an entry for ProbeValues to be collected
								probes.push_back(ProbeValues{});
								ping(probes.size() - 1);
							}, std::chrono::seconds{1}));
						}

						// sending SlimProto Stop command, which will flush client's buffer and will respond with initiate STAT/STMf message, which will change state to Started
						send(server::CommandSTRM{CommandSelection::Stop});

						// changing state to Draining
						stateMachine.processEvent(HandshakeEvent, [&](auto event, auto state)
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Handshake event - closing the connection";
							connection.get().stop();
						});
					}
					else
					{
						LOG(WARNING) << LABELS{"proto"} << "Multiple HELO command received - skipped";
					}

					return result;
				}

				inline auto onRESP()
				{
					std::size_t result{0};

					LOG(INFO) << LABELS{"proto"} << "RESP command received";

					// deserializing RESP command
					result = client::CommandRESP{commandRingBuffer}.getSize();

					return result;
				}

				inline auto onSETD()
				{
					std::size_t result{0};

					LOG(INFO) << LABELS{"proto"} << "SETD command received";

					// deserializing SETD command
					result = client::CommandSETD{commandRingBuffer}.getSize();

					return result;
				}

				inline auto onSTAT(util::Timestamp receiveTimestamp)
				{
					auto commandSTAT{client::CommandSTAT{commandRingBuffer}};
					auto result{commandSTAT.getSize()};

					auto event{std::string{commandSTAT.getData()->event, 4}};
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
					// change state to Started
					stateMachine.processEvent(FlushedEvent, [&](auto event, auto state)
					{
						LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Flushed event";
					});
				}

				inline void onSTMl(client::CommandSTAT& commandSTAT)
				{
					clientBufferIsReady = true;

					auto bufferSize{commandSTAT.getData()->streamBufferSize};
					auto fullness{commandSTAT.getData()->streamBufferFullness};

					LOG(DEBUG) << LABELS{"proto"}
							<<  "bufferSize=" << bufferSize
							<< " fullness="   << fullness << " (" << 100 * fullness / bufferSize << "%)";
				}

				inline void onSTMs(client::CommandSTAT& commandSTAT)
				{
					auto bufferSize{commandSTAT.getData()->streamBufferSize};
					auto fullness{commandSTAT.getData()->streamBufferFullness};

					LOG(DEBUG) << LABELS{"proto"}
							<<  "bufferSize=" << bufferSize
							<< " fullness="   << fullness << " (" << 100 * fullness / bufferSize << "%)";
				}

				inline void onSTMt(client::CommandSTAT& commandSTAT, util::Timestamp receiveTimestamp)
				{
					// processing guard: skipping any requests which were not originated by the server
					if (!commandSTAT.getData()->serverTimestamp || commandSTAT.getData()->serverTimestamp > probes.capacity())
					{
						return;
					}

					auto  index{commandSTAT.getData()->serverTimestamp - 1};
					auto& probe{probes[index]};
					auto  moreProbesNeeded{true};

					// latency is calculated assuming that it equals to network round-trip / 2
					probe.clientTimestamp = util::Timestamp{util::Duration{((std::uint64_t)commandSTAT.getData()->jiffies) * 1000}};
					probe.clientDuration  = util::Duration{((std::uint64_t)commandSTAT.getData()->elapsedMilliseconds) * 1000};
					probe.latency         = (receiveTimestamp - probe.sendTimestamp) / 2;

					// if latency measurement was not interfered by other commands then probe once again
					if (measuringLatency)
					{
						auto accurateDurationIndex{ts::optional<std::size_t>{ts::nullopt}};

						// if there is enough samples to calculate mean values
						if (probes.size() == probes.capacity())
						{
							// checking if there is an 'accurate' client playback duration sample available
							if (stateMachine.state == PlayingState)
							{
								accurateDurationIndex = getAccurateDurationIndex();
							}
						}

						// if required samples are available so that latency and time offset can be calculated
						if ((stateMachine.state != PlayingState && probes.size() == probes.capacity())
						||  (stateMachine.state == PlayingState && accurateDurationIndex.value_or(0) >= 3))
						{
							// adjusting latency and time offset values
							auto meanProbe{getLatencyMeanProbe()};
							auto meanProbeOffset{meanProbe.sendTimestamp - meanProbe.clientTimestamp};
							timeOffset = (timeOffset.value_or(meanProbeOffset) * 8 + meanProbeOffset   * 2) / 10;
							latency    = (latency.value_or(meanProbe.latency)  * 8 + meanProbe.latency * 2) / 10;

							LOG(DEBUG) << LABELS{"proto"} << "Client latency was calculated"
								<< " (client id=" << clientID
								<< ", latency=" << latency.value().count() << " microsec)";

							if (accurateDurationIndex.value_or(0) >= 3)
							{
								// calculating timing at the point of send request event
								auto& accurateDurationProbe{probes[accurateDurationIndex.value()]};
								auto  playbackDiff{std::chrono::abs(accurateDurationProbe.clientDuration - accurateDurationProbe.latency - streamer.get().calculateDuration(lastChunkCapturedFrames, util::microseconds))};
								auto  timeDiff{std::chrono::abs(accurateDurationProbe.sendTimestamp - lastChunkTimestamp)};

								if (playbackDriftBase.has_value())
								{
									auto playbackDrift{playbackDriftBase.value() - playbackDiff - timeDiff};

									LOG(DEBUG) << LABELS{"proto"}
										<< "Client playback drift was calculated (client id=" << clientID
										<< ", drift=" << playbackDrift.count() << " microsec)";
								}
								else
								{
									playbackDriftBase = playbackDiff + timeDiff;
								}
							}

							// clearing the buffer so samples from previous probing do not impact the result
							probes.clear();
							moreProbesNeeded = false;
						}
						else
						{
							// adding a new entry for ProbeValues to be collected
							probes.push_back(ProbeValues{});
							index = probes.size() - 1;

						}
					}
					else
					{
						LOG(DEBUG) << LABELS{"proto"} << "Latency probe was skipped due to interfering with other requests";
					}

					if (moreProbesNeeded)
					{
						// capturing index by value
						processorProxy.process([&, index{index}]
						{
							ping(index);
						});
					}
					else
					{
						// TODO: make it configurable
						// TODO: make it resistant to clients 'loosing' requests
						// submitting a new ping request if there is no other pending request
						if (!pingTimer.has_value())
						{
							pingTimer = ts::ref(processorProxy.processWithDelay([&]
							{
								// releasing timer so a new 'delayed' request may be issued
								pingTimer.reset();

								// allocating an entry for ProbeValues to be collected
								probes.push_back(ProbeValues{});
								ping(probes.size() - 1);
							}, std::chrono::seconds{5}));
						}
					}
				}

				inline void onSTMu(client::CommandSTAT& commandSTAT)
				{
					if (stateMachine.state == DrainingState)
					{
						// sending SlimProto Stop command which will flush client's buffer, which will cause STAT/STMf response
						send(server::CommandSTRM{CommandSelection::Stop});
					}
				}

				inline void ping(std::uint32_t index)
				{
					// creating a ping command
					auto command{server::CommandSTRM{CommandSelection::Time}};
					command.getBuffer()->data.replayGain = index + 1;

					// capturing server timestamp as close as possible to the send operation
					auto timestamp{util::Timestamp::now()};

					// sending actual ping command
					send(command);

					// changing state to 'measuringLatency' which is used to track if there any interference while measuring latency
					measuringLatency = true;

					// storing actually sent timestamp in the cache
					probes[index].sendTimestamp = timestamp;
				}

				template<typename CommandType>
				inline void send(CommandType&& command)
				{
					measuringLatency = false;

					// TODO: use local buffer and async API to prevent from interfering while measuring latency
					connection.get().write(command.getBuffer(), command.getSize());
				}

				inline void stateChangeToPlaying()
				{
					// no need to account for latency here as Streamer class adds max latency per all session while evaluating playback start time
					send(server::CommandSTRM{CommandSelection::Unpause, streamer.get().getPlaybackStartTime() - timeOffset.value_or(util::Duration{0})});
				}

				inline void stateChangeToPreparing()
				{
					// resetting flag indicating whether client has buffered enough to start playback
					clientBufferIsReady = false;

					// resetting reference to an old HTTP session as a new session will be created
					streamingSession.reset();

					// resetting drift base value as it should be calculated for every new playback
					playbackDriftBase.reset();

					send(server::CommandSTRM{CommandSelection::Start, formatSelection, streamingPort, samplingRate, clientID});
				}

				inline void stateChangeToStopped()
				{
					// resetting sampling rate which will be provided to prepare(...) method
					samplingRate = 0;
				}

			private:
				conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>>         processorProxy;
				std::reference_wrapper<ConnectionType>                           connection;
				std::reference_wrapper<StreamerType>                             streamer;
				std::string                                                      clientID;
				unsigned int                                                     streamingPort;
				FormatSelection                                                  formatSelection;
				ts::optional<unsigned int>                                       gain;
				CommandHandlersMap                                               commandHandlers;
				EventHandlersMap                                                 eventHandlers;
				util::StateMachine<Event, State>                                 stateMachine;
				unsigned int                                                     samplingRate{0};
				ts::optional_ref<StreamingSession<ConnectionType, StreamerType>> streamingSession{ts::nullopt};
				// TODO: parameterize
				util::buffer::Ring<std::uint8_t>                                 commandRingBuffer{2048};
				ts::optional<client::CommandHELO>                                commandHELO{ts::nullopt};
				ts::optional_ref<conwrap2::Timer>                                pingTimer{ts::nullopt};
				bool                                                             measuringLatency{false};
				ts::optional<util::Duration>                                     latency;
				ts::optional<util::Duration>                                     timeOffset;
				// TODO: parameterize
				std::vector<ProbeValues>                                         probes{13};
				ts::optional<util::Duration>                                     playbackDriftBase{ts::nullopt};
				bool                                                             clientBufferIsReady{false};
				util::Timestamp                                                  lastChunkTimestamp;
				util::BigInteger                                                 lastChunkCapturedFrames{0};
		};
	}
}
