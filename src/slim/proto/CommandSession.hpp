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
#include <cstddef>  // std::size_t
#include <functional>
#include <optional>
#include <scope_guard.hpp>
#include <string>
#include <type_safe/optional.hpp>

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

		struct LatencyMeasurement
		{
			LatencyMeasurement(std::uint32_t k)
			: serverTimestampKey{k} {}

			std::uint32_t               serverTimestampKey;
			bool                        serverTimestampCaptured{false};
			ts::optional<std::uint32_t> clientTimestamp{ts::nullopt};
		};

		template<typename ConnectionType>
		class CommandSession
		{
			using CommandHandlersMap   = std::unordered_map<std::string, std::function<std::size_t(unsigned char*, std::size_t, util::Timestamp)>>;
			using EventHandlersMap     = std::unordered_map<std::string, std::function<void(client::CommandSTAT&, util::Timestamp)>>;
			using StreamingSessionType = StreamingSession<ConnectionType>;

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
									latencyMeasurement.reset();
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

					ping();

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
							latencyMeasurement.reset();
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

					//LOG(DEBUG) << LABELS{"proto"} << "PONG timestampKey=" << timestampKey;

					// if request originated by a server then use it for measuring latency

					LOG(WARNING) << LABELS{"proto"} << "Ping11";

					if (timestampKey && latencyMeasurement.has_value() && timestampKey == latencyMeasurement.value().serverTimestampKey)
					{
						LOG(WARNING) << LABELS{"proto"} << "Ping12";
						if (!latencyMeasurement.value().clientTimestamp.has_value())
						{
							LOG(WARNING) << LABELS{"proto"} << "Ping13";

							latencyMeasurement.value().clientTimestamp = commandSTAT.getBuffer()->jiffies;

							auto sendTimestamp = timestampCache.load(timestampKey);
							LOG(DEBUG) << LABELS{"proto"} << "PONG sendTimestamp=" << sendTimestamp.value().getMicroSeconds();
							LOG(DEBUG) << LABELS{"proto"} << "PONG clientTimestamp=" << commandSTAT.getBuffer()->jiffies;
						}
/*
						if (sendTimestamp.has_value())
						{

							// TODO: work in progress
							if (!difff)
							{
								difff = sendTimestamp.value().getMilliSeconds() - commandSTAT.getBuffer()->jiffies;
							}
							else
							{
								auto d{difff};
								d = sendTimestamp.value().getMilliSeconds() - commandSTAT.getBuffer()->jiffies;
								LOG(DEBUG) << LABELS{"proto"} << "PONG diff=" << difff - d;
							}

							if (latencyMeasurement.has_value())
							{
								if (latency.has_value())
								{
									LOG(DEBUG) << LABELS{"proto"} << "PONG latency=" << latency.value();

									auto l1 = latency.value();
									auto l2 = (receiveTimestamp.getMicroSeconds() - sendTimestamp.value().getMicroSeconds()) / 2;
									latency = l1 * 8 / 10 + l2 * 2 / 10;

									LOG(DEBUG) << LABELS{"proto"} << "PONG new latency=" << l2;
								}
								else
								{
									latency = (receiveTimestamp.getMicroSeconds() - sendTimestamp.value().getMicroSeconds()) / 2;
								}
								LOG(DEBUG) << LABELS{"proto"} << "Client latency=" << latency.value();

								//auto n = std::chrono::steady_clock::now();
								//auto d = std::chrono::duration_cast<std::chrono::milliseconds>(n.time_since_epoch()).count();
								//LOG(DEBUG) << LABELS{"proto"} << "jiffies=" << commandSTAT.getBuffer()->jiffies;
								//LOG(DEBUG) << LABELS{"proto"} << "server=" << d;
							}
						}
						else
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid server timestamp data sent by a client (key received=" << commandSTAT.getBuffer()->serverTimestamp << ")";
						}
*/
/*
						// collecting enough round-trip samples to measure latency
						if (timestampCache.elements() < timestampCache.size())
						{
							ping();
						}
						else
						{
							processorProxy.processWithDelay([&](auto& context)
							{
								ping();
							}, std::chrono::seconds{5});
						}
*/
					}
				}

				inline void ping()
				{
					LOG(WARNING) << LABELS{"proto"} << "Ping1";

					// creating latency measument data
					latencyMeasurement = LatencyMeasurement{timestampCache.store()};

					// start measuring latency
					sendPing();
				}

				template<typename CommandType>
				inline void send(CommandType&& command)
				{
					latencyMeasurement.reset();

					// TODO: use local buffer and async API to prevent from interfering while measuring latency
					connection.get().write(command.getBuffer(), command.getSize());
				}

				inline void sendPing()
				{
					LOG(WARNING) << LABELS{"proto"} << "Ping2";
					if (latencyMeasurement.has_value())
					{
						LOG(WARNING) << LABELS{"proto"} << "Ping3";

						// creating ping command
						auto command{server::CommandSTRM{CommandSelection::Time}};
						command.getBuffer()->data.replayGain = latencyMeasurement.value().serverTimestampKey;

						connection.get().setNoDelay(true);
						connection.get().setQuickAcknowledgment(true);

						// capturing server timestamp as close as possible to the send operation
						ts::optional<util::Timestamp> timestamp{ts::nullopt};
						if (!latencyMeasurement.value().serverTimestampCaptured)
						{
							timestamp = util::Timestamp{};
						}

						// sending actual ping command
						connection.get().write(command.getBuffer(), command.getSize());
						connection.get().setNoDelay(true);
						connection.get().setQuickAcknowledgment(true);

						if (timestamp.has_value())
						{
							timestampCache.update(latencyMeasurement.value().serverTimestampKey, util::Timestamp{});
							latencyMeasurement.value().serverTimestampCaptured = true;
						}

						// keep sending ping command until client responds
						if (!latencyMeasurement.value().clientTimestamp.has_value())
						{
							processorProxy.processWithDelay([&]
							{
								sendPing();
							}, std::chrono::milliseconds{1});
						}
					}
				}

			private:
				conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy;
				std::reference_wrapper<ConnectionType>                   connection;
				std::string                                              clientID;
				unsigned int                                             streamingPort{0};
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
				ts::optional<LatencyMeasurement>                         latencyMeasurement{ts::nullopt};

				util::TimestampCache<10>                                 timestampCache;
				ts::optional<unsigned int>                               latency{ts::nullopt};
				signed long long                                         difff{0};
		};
	}
}
