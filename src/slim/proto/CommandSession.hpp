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
#include <memory>
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
#include "slim/util/Buffer.hpp"
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
			using CommandHandlersMap = std::unordered_map<std::string, std::function<std::size_t(unsigned char*, std::size_t, util::Timestamp)>>;
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

				inline auto getClientID()
				{
					return clientID;
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
					if (!isRunning())
					{
						return;
					}

					// removing data from the buffer in case of an exception
					::util::scope_guard_failure onException = [&]
					{
						commandBuffer.clear();
					};

					// adding data to the buffer
					commandBuffer.addData(buffer, size);

					std::size_t keySize{4};
					std::size_t processedSize;
					do
					{
						processedSize = 0;

						// searching for a SlimProto Command based on a label in the buffer
						auto label{std::string{(char*)commandBuffer.getData(), commandBuffer.getDataSize() > keySize ? keySize : 0}};
						auto found{commandHandlers.find(label)};
						if (found != commandHandlers.end())
						{
							// if there is enough data to process this message
							auto data{commandBuffer.getData()};
							auto size{commandBuffer.getDataSize()};
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
								// TODO: shrinking memory means moving data towards the beginning of the buffer; RingBuffer should be used
								shrinkBufferLeft(processedSize);
							}
						}
						else if (label.length() > 0)
						{
							LOG(WARNING) << LABELS{"proto"} << "Unsupported SlimProto command received (header='" << label << "')";

							commandBuffer.clear();
						}
					} while (processedSize > 0);
				}

				inline auto prepare(const unsigned int& s)
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
					// changing state to Running if needed
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

				inline void streamChunk(const Chunk& chunk)
				{
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
							streamingSession.streamChunk(chunk);
						});

						if (chunk.isEndOfStream())
						{
							// changing state to Draining
							stateMachine.processEvent(DrainEvent, [&](auto event, auto state)
							{
								LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Drain event - closing the connection";
								connection.get().stop();
							});
						}
					}
				}

			protected:
				template <typename DurationType>
				inline auto calculateMean(std::vector<DurationType>& samples)
				{
					DurationType result{0};

					// sorting samples
					std::sort(samples.begin(), samples.end());

					// making sure there is enough sample
					if (samples.size() > 7)
					{
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
						if (!pingTimer.has_value())
						{
							pingTimer = ts::ref(processorProxy.processWithDelay([&]
							{
								// releasing timer so a new 'delayed' request may be issued
								pingTimer.reset();

								// creating a timestamp cache entry required to allocate a key
								ping(timestampCache.store(util::Timestamp::now()));
							}, std::chrono::seconds{1}));
						}

						// sending SlimProto Stop command, which will flush client's buffer and will respond with initiate STAT/STMf message, which will change state to Running
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
					// change state to Running
					stateMachine.processEvent(FlushedEvent, [&](auto event, auto state)
					{
						LOG(WARNING) << LABELS{"proto"} << "Invalid SlimProto session state while processing Flushed event";
					});
				}

				inline void onSTMl(client::CommandSTAT& commandSTAT)
				{
					clientBufferIsReady = true;

					auto bufferSize{commandSTAT.getBuffer()->streamBufferSize};
					auto fullness{commandSTAT.getBuffer()->streamBufferFullness};
					LOG(DEBUG) << LABELS{"proto"}
							<<  "bufferSize=" << bufferSize
							<< " fullness="   << fullness << " (" << 100 * fullness / bufferSize << "%)";
				}

				inline void onSTMs(client::CommandSTAT& commandSTAT)
				{
					auto bufferSize{commandSTAT.getBuffer()->streamBufferSize};
					auto fullness{commandSTAT.getBuffer()->streamBufferFullness};
					LOG(DEBUG) << LABELS{"proto"}
							<<  "bufferSize=" << bufferSize
							<< " fullness="   << fullness << " (" << 100 * fullness / bufferSize << "%)";

					LOG(DEBUG) << LABELS{"proto"} << "client played=" << commandSTAT.getBuffer()->elapsedMilliseconds;

					ts::with(streamingSession, [&](auto& streamingSession)
					{
						if (samplingRate)
						{
							LOG(DEBUG) << LABELS{"proto"} << "server played=" << ((streamingSession.getFramesProvided() / samplingRate) * 1000);
						}
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
							// latency is calculated assuming that server->client part takes 66% of a round-trip
							auto latencySample{std::chrono::duration_cast<std::chrono::microseconds>(((receiveTimestamp - sendTimestamp) * 2) / 3)};

							// saving latency sample for further processing
							latencySamples.push_back(latencySample);
							timeOffsetSamples.emplace_back(sendTimestamp.get(util::milliseconds) - commandSTAT.getBuffer()->jiffies);

							// if there is enough samples to calculate latency
							if (timestampCache.size() >= timestampCache.capacity())
							{
								// calculating new latency value
								auto meanLatency{calculateMean(latencySamples)};
								auto meanTimeOffset{calculateMean(timeOffsetSamples)};

								// updating current latency value
								latency    = (latency.value_or(meanLatency)       * 8 + meanLatency    * 2) / 10;
								timeOffset = (timeOffset.value_or(meanTimeOffset) * 8 + meanTimeOffset * 2) / 10;
								LOG(DEBUG) << LABELS{"proto"} << "Client latency updated (client id=" << clientID << ", latency=" << latency.value().count() << " microsec)";

								// clearing the cache so it can be used for collecting new samples
								timestampCache.clear();
								latencySamples.clear();
								timeOffsetSamples.clear();

								// TODO: work in progress
								ts::with(streamingSession, [&](auto& streamingSession)
								{
									// elapsed_milliseconds vs ???
									// getStreamingDuration - getBufferingDuration - ???
									// 
									// getServerTime(jiffies) - getPlaybackStartTime should be ~equal to elapsed_milliseconds
									//streamingSession.driftCorrection();
								});

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

				inline void onSTMu(client::CommandSTAT& commandSTAT)
				{
					if (stateMachine.state == DrainingState)
					{
						// sending SlimProto Stop command which will flush client's buffer, which will cause STAT/STMf response
						send(server::CommandSTRM{CommandSelection::Stop});
					}
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

					// storing actually sent timestamp in the cache
					timestampCache.update(timestampKey, timestamp);
				}

				template<typename CommandType>
				inline void send(CommandType&& command)
				{
					measuringLatency = false;

					// TODO: use local buffer and async API to prevent from interfering while measuring latency
					connection.get().write(command.getBuffer(), command.getSize());
				}

				inline void shrinkBufferLeft(std::size_t pos)
				{
					if (pos >= commandBuffer.getDataSize())
					{
						commandBuffer.clear();
					}
					else if (pos > 0)
					{
						commandBuffer.setDataSize(commandBuffer.getDataSize() - pos);
						std::memcpy(commandBuffer.getData(), commandBuffer.getData() + pos, commandBuffer.getDataSize());
					}
				}

				inline void stateChangeToPlaying()
				{
					auto playbackStartedAt = streamer.get().getPlaybackStartTime();

					ts::with(timeOffset, [&](auto& timeOffset)
					{
						send(server::CommandSTRM{CommandSelection::Unpause, playbackStartedAt - timeOffset});
					});
				}

				inline void stateChangeToPreparing()
				{
					// resetting flag indicating whether client has buffered enough to start playback
					clientBufferIsReady = false;

					// resetting reference to an old HTTP session as a new session will be created
					streamingSession.reset();

					send(server::CommandSTRM{CommandSelection::Start, formatSelection, streamingPort, samplingRate, clientID});
				}

				inline void stateChangeToStopped()
				{
					samplingRate = 0;
					measuringLatency = false;
					clientBufferIsReady = false;

					streamingSession.reset();
					commandBuffer.clear();
					timestampCache.clear();
					latency.reset();
					latencySamples.clear();
					timeOffset.reset();
					timeOffsetSamples.clear();

					ts::with(pingTimer, [&](auto& timer)
					{
						timer.cancel();
					});
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
				// TODO: parametrize
				util::Buffer                                                     commandBuffer{2048};
				ts::optional<client::CommandHELO>                                commandHELO{ts::nullopt};
				ts::optional_ref<conwrap2::Timer>                                pingTimer{ts::nullopt};
				util::ArrayCache<util::Timestamp, 10>                            timestampCache;
				bool                                                             measuringLatency{false};
				ts::optional<std::chrono::microseconds>                          latency{ts::nullopt};
				std::vector<std::chrono::microseconds>                           latencySamples;
				ts::optional<std::chrono::milliseconds>                          timeOffset{ts::nullopt};
				std::vector<std::chrono::milliseconds>                           timeOffsetSamples;
				bool                                                             clientBufferIsReady{false};
		};
	}
}
