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

#include <chrono>
#include <conwrap2/ProcessorProxy.hpp>
#include <conwrap2/Timer.hpp>
#include <cstddef>  // std::size_t
#include <functional>
#include <scope_guard.hpp>
#include <string>
#include <type_safe/optional.hpp>
#include <type_safe/optional_ref.hpp>

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
#include "slim/util/ExpandableBuffer.hpp"
#include "slim/util/Timestamp.hpp"
#include "slim/util/TimestampCache.hpp"


namespace slim
{
	namespace proto
	{
		namespace ts = type_safe;

		template<typename ConnectionType>
		class CommandSession
		{
			using CommandHandlersMap   = std::unordered_map<std::string, std::function<std::size_t(unsigned char*, std::size_t, util::Timestamp)>>;
			using EventHandlersMap     = std::unordered_map<std::string, std::function<void(client::CommandSTAT&, util::Timestamp)>>;
			using StreamingSessionType = StreamingSession<ConnectionType>;
			using LatencyType          = unsigned long long;

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
					{"STMc", [&](auto& commandSTAT, auto timestamp) {onSTMc(commandSTAT);}},
					{"STMd", [&](auto& commandSTAT, auto timestamp) {}},
					{"STMf", [&](auto& commandSTAT, auto timestamp) {}},
					{"STMl", [&](auto& commandSTAT, auto timestamp) {onSTMl(commandSTAT);}},
					{"STMo", [&](auto& commandSTAT, auto timestamp) {}},
					{"STMr", [&](auto& commandSTAT, auto timestamp) {}},
					{"STMs", [&](auto& commandSTAT, auto timestamp) {onSTMs(commandSTAT);}},
					{"STMt", [&](auto& commandSTAT, auto timestamp) {onSTMt(commandSTAT, timestamp);}},
					{"STMu", 0},
				}
				{
					LOG(DEBUG) << LABELS{"proto"} << "SlimProto session object was created (id=" << this << ")";
				}

				~CommandSession()
				{
					// canceling deferred operation if any
					pingTimer.map([&](auto& timer)
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

				inline auto getStreamingSession()
				{
					return streamingSessionPtr;
				}

				inline auto getSamplingRate()
				{
					return samplingRate;
				}

				inline auto isConnectedReceived()
				{
					return connectedReceived;
				}

				inline auto isResponseReceived()
				{
					return responseReceived;
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

				inline void setSamplingRate(unsigned int s)
				{
					// TODO: implement proper processing
					samplingRate = s;
				}

				inline void setStreamingSession(StreamingSessionType* s)
				{
					streamingSessionPtr = s;
					if (!streamingSessionPtr)
					{
						connectedReceived = false;
						responseReceived  = false;
					}
				}

				inline void start()
				{
					// if not streaming and handshake was received
					if (!streaming && commandHELO.has_value())
					{
						send(server::CommandSTRM{CommandSelection::Start, formatSelection, streamingPort, samplingRate, clientID});
					}
					streaming = true;
				}

				inline void stop()
				{
					// if streaming and handshake was received
					if (streaming && commandHELO.has_value())
					{
						send(server::CommandSTRM{CommandSelection::Stop});
					}
					streaming = false;
				}

			protected:
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
					std::size_t result{0};

					LOG(INFO) << LABELS{"proto"} << "HELO command received";

					// deserializing HELO command
					commandHELO = client::CommandHELO{buffer, size};
					result      = commandHELO.value().getSize();

					send(server::CommandSETD{server::DeviceID::RequestName});
					send(server::CommandSETD{server::DeviceID::Squeezebox3});
					send(server::CommandAUDE{true, true});
					send(server::CommandAUDG{gain});

					// sending SlimProto Stop command will flush client's buffer which will initiate STAT/STMf message
					send(server::CommandSTRM{CommandSelection::Stop});

					// start sending ping commands after the 'handshake' commands
					pingTimer = ts::ref(processorProxy.processWithDelay([&]
					{
						ping();
					}, std::chrono::seconds{1}));

					// TODO: work in progress
					if (streaming)
					{
						send(server::CommandSTRM{CommandSelection::Start, formatSelection, streamingPort, samplingRate, clientID});
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

				inline auto onSTAT(unsigned char* buffer, std::size_t size, util::Timestamp timestamp)
				{
					client::CommandSTAT commandSTAT{buffer, size};
					std::size_t         result{commandSTAT.getSize()};

					auto event{commandSTAT.getEvent()};
					auto found{eventHandlers.find(event)};
					if (found != eventHandlers.end())
					{
						LOG(DEBUG) << LABELS{"proto"} << event << " event received";

						// if this is not STMt event then reseting measuring flag
						// TODO: consider proper usage of STAT event label
						if (event.compare("STMt") != 0)
						{
							measuringLatency = false;
						}

						// invoking STAT event handler
						(*found).second(commandSTAT, timestamp);
					}
					else
					{
						LOG(DEBUG) << LABELS{"proto"} << "Unsupported STAT event received: " << event;
					}

					return result;
				}

				inline void onSTMc(client::CommandSTAT& commandSTAT)
				{
					connectedReceived = true;
				}

				inline void onSTMl(client::CommandSTAT& commandSTAT)
				{
					send(server::CommandSTRM{CommandSelection::Unpause});
				}

				inline void onSTMs(client::CommandSTAT& commandSTAT)
				{
					// TODO: work in progress
					LOG(DEBUG) << LABELS{"proto"} << "client played=" << commandSTAT.getBuffer()->elapsedMilliseconds;
					if (streamingSessionPtr)
					{
						LOG(DEBUG) << LABELS{"proto"} << "server played=" << ((streamingSessionPtr->getFramesProvided() / samplingRate) * 1000);
					}
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
					sendTimestamp.map([&](auto& sendTimestamp)
					{
						// if latency measurement was interfered by other commands then repeat round trip once again
						if (!measuringLatency)
						{
							LOG(DEBUG) << LABELS{"proto"} << "Latency probe was skipped";

							// capturing timestamp key by value
							processorProxy.process([&, timestampKey{timestampKey}]
							{
								sendPing(timestampKey);
							});
						}
						else
						{
							// saving latency sample for further processing
							latencySamples.push_back((receiveTimestamp.getMicroSeconds() - sendTimestamp.getMicroSeconds()) / 2);

							if (timestampCache.size() < timestampCache.capacity())
							{
								processorProxy.process([&]
								{
									ping();
								});
							}
							else
							{
								// TODO: calculate avg latency
								// ordering latencies
								std::sort(latencySamples.begin(), latencySamples.end());

								std::for_each(latencySamples.begin(), latencySamples.end(), [](auto& latency)
								{
									LOG(DEBUG) << LABELS{"proto"} << "latency=" << latency;
								});

								if (latencySamples.size() > 7)
								{
									LatencyType accumulator{0};
									for (std::size_t i{2}; i < latencySamples.size() - 2; i++)
									{
										accumulator += latencySamples[i];
									}
									accumulator /= latencySamples.size() - 4;
									
									LOG(DEBUG) << LABELS{"proto"} << "avg latency=" << accumulator;
								}

								// clearing the cache so it can be used for collecting a new sample
								timestampCache.clear();
								latencySamples.clear();

								// TODO: make it resistant to clients 'loosing' requests
								// submitting a new ping request
								pingTimer = ts::ref(processorProxy.processWithDelay([&]
								{
									ping();
								}, std::chrono::seconds{5}));
							}
						}

						LOG(DEBUG) << LABELS{"proto"} << "PONG sendTimestamp="   << sendTimestamp.getMicroSeconds();
						LOG(DEBUG) << LABELS{"proto"} << "PONG clientTimestamp=" << commandSTAT.getBuffer()->jiffies;
						LOG(DEBUG) << LABELS{"proto"} << "PONG new latency="     << (receiveTimestamp.getMicroSeconds() - sendTimestamp.getMicroSeconds()) / 2;
					});
				}

				inline void ping()
				{
					LOG(DEBUG) << LABELS{"proto"} << "Ping1";

					pingTimer.reset();

					// creating a timestamp cache entry required to allocate a key
					sendPing(timestampCache.store());
				}

				template<typename CommandType>
				inline void send(CommandType&& command)
				{
					measuringLatency = false;

					// TODO: use local buffer and async API to prevent from interfering while measuring latency
					connection.get().write(command.getBuffer(), command.getSize());
				}

				inline void sendPing(std::uint32_t timestampKey)
				{
					// creating a ping command
					auto command{server::CommandSTRM{CommandSelection::Time}};
					command.getBuffer()->data.replayGain = timestampKey;

					// capturing server timestamp as close as possible to the send operation
					util::Timestamp timestamp;

					// sending actual ping command
					send(command);

					// changing state to 'measuringLatency' which is used to track if there any interference while measuring latency
					measuringLatency = true;

					// storing the actual send timestamp in the cache
					timestampCache.update(timestampKey, timestamp);
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
				bool                                                     streaming{false};
				unsigned int                                             samplingRate{0};
				StreamingSessionType*                                    streamingSessionPtr{nullptr};
				bool                                                     connectedReceived{false};
				bool                                                     responseReceived{false};
				util::ExpandableBuffer                                   commandBuffer{std::size_t{0}, std::size_t{2048}};
				ts::optional<client::CommandHELO>                        commandHELO{ts::nullopt};
				util::TimestampCache<10>                                 timestampCache;
				ts::optional<LatencyType>                                latency{ts::nullopt};
				std::vector<LatencyType>                                 latencySamples;
				bool                                                     measuringLatency{false};
				ts::optional_ref<conwrap2::Timer>                        pingTimer{ts::nullopt};
		};
	}
}
