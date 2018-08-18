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

#include <cstddef>  // std::size_t
#include <functional>
#include <optional>
#include <scope_guard.hpp>
#include <string>

#include "slim/Exception.hpp"
#include "slim/log/log.hpp"
#include "slim/proto/CommandAUDE.hpp"
#include "slim/proto/CommandAUDG.hpp"
#include "slim/proto/CommandHELO.hpp"
#include "slim/proto/CommandRESP.hpp"
#include "slim/proto/CommandSETD.hpp"
#include "slim/proto/CommandSTAT.hpp"
#include "slim/proto/CommandSTRM.hpp"
#include "slim/proto/StreamingSession.hpp"
#include "slim/util/ExpandableBuffer.hpp"
#include "slim/util/Timestamp.hpp"
#include "slim/util/TimestampCache.hpp"


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType>
		class CommandSession
		{
			using CommandHandlersMap   = std::unordered_map<std::string, std::function<std::size_t(unsigned char*, std::size_t, util::Timestamp)>>;
			using EventHandlersMap     = std::unordered_map<std::string, std::function<void(CommandSTAT&, util::Timestamp)>>;
			using StreamingSessionType = StreamingSession<ConnectionType>;

			public:
				CommandSession(std::reference_wrapper<ConnectionType> co, std::string id, unsigned int p, FormatSelection f, std::optional<unsigned int> g)
				: connection{co}
				, clientID{id}
				, streamingPort{p}
				, formatSelection{f}
				, gain{g}
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
					// removing data from the buffer in case of an error
					::util::scope_guard_failure onError = [&]
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

				inline void ping()
				{
					auto& nativeSocket{connection.get().getNativeSocket()};

					if (nativeSocket.is_open())
					{
						auto          command{CommandSTRM{CommandSelection::Time}};
						auto          buffer{command.getBuffer()};
						auto          size{command.getSize()};
						std::uint32_t timestampKey{0};
						int           result{0};
						std::size_t   sent{0};

						auto onError = [&]
						{
							// deleting a new timestamp entry if it was created
							if (timestampKey > 0)
							{
								timestampCache.erase(timestampKey);
							}
						};
						::util::scope_guard_failure onErrorGuard{onError};

						timestampKey = timestampCache.create();
						command.getBuffer()->data.replayGain = timestampKey;

						// setting measuring flag to make sure there were no other commends processed inbetween
						measuringLatency = true;

						// capturing ping time as close as possible to the moment of sending data out
						util::Timestamp timestamp;

						// sending 'ping' command as close as possible to the local time capture point
						while (result >= 0 && sent < size)
						{
							result  = nativeSocket.send(asio::buffer(buffer + sent, size - sent));
							sent   += (result >= 0 ? result : 0);
						}

						// saving actual timestamp; otherwise - handling send error
						if (result >= 0)
						{
							timestampCache.update(timestampKey, timestamp);
						}
						else
						{
							onError();
						}
					}
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
						send(CommandSTRM{CommandSelection::Start, formatSelection, streamingPort, samplingRate, clientID});
					}
					streaming = true;
				}

				inline void stop()
				{
					// if streaming and handshake was received
					if (streaming && commandHELO.has_value())
					{
						send(CommandSTRM{CommandSelection::Stop});
					}
					streaming = false;
				}

			protected:
				inline auto onDSCO(unsigned char* buffer, std::size_t size)
				{
					LOG(INFO) << LABELS{"proto"} << "DSCO command received";

					// TODO: work in progress
					std::size_t result{0};
					std::size_t offset{4};

					if (size >= offset + sizeof(std::uint32_t))
					{
						result = ntohl(*(std::uint32_t*)(buffer + offset)) + offset + sizeof(std::uint32_t);
					}

					return result;
				}

				inline auto onHELO(unsigned char* buffer, std::size_t size)
				{
					std::size_t result{0};

					LOG(INFO) << LABELS{"proto"} << "HELO command received";

					// deserializing HELO command
					commandHELO = CommandHELO{buffer, size};
					result      = commandHELO.value().getSize();

					send(CommandSETD{DeviceID::RequestName});
					send(CommandSETD{DeviceID::Squeezebox3});
					send(CommandAUDE{true, true});
					send(CommandAUDG{gain});

					// sending SlimProto Stop command will flush client's buffer which will initiate STAT/STMf message
					send(CommandSTRM{CommandSelection::Stop});

					ping();

					// TODO: work in progress
					if (streaming)
					{
						send(CommandSTRM{CommandSelection::Start, formatSelection, streamingPort, samplingRate, clientID});
					}

					return result;
				}

				inline auto onRESP(unsigned char* buffer, std::size_t size)
				{
					std::size_t result{0};

					LOG(INFO) << LABELS{"proto"} << "RESP command received";

					// deserializing RESP command
					result = CommandRESP{buffer, size}.getSize();

					return result;
				}

				inline auto onSETD(unsigned char* buffer, std::size_t size)
				{
					LOG(INFO) << LABELS{"proto"} << "SETD command received";

					// TODO: work in progress
					std::size_t result{0};
					std::size_t offset{4};

					if (size >= offset + sizeof(std::uint32_t))
					{
						result = ntohl(*(std::uint32_t*)(buffer + offset)) + offset + sizeof(std::uint32_t);
					}

					return result;
				}

				inline auto onSTAT(unsigned char* buffer, std::size_t size, util::Timestamp timestamp)
				{
					CommandSTAT commandSTAT{buffer, size};
					std::size_t result{commandSTAT.getSize()};

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

				inline void onSTMc(CommandSTAT& commandSTAT)
				{
					connectedReceived = true;
				}

				inline void onSTMl(CommandSTAT& commandSTAT)
				{
					send(CommandSTRM{CommandSelection::Unpause});
				}

				inline void onSTMs(CommandSTAT& commandSTAT)
				{
					// TODO: work in progress
					LOG(DEBUG) << LABELS{"proto"} << "client played=" << commandSTAT.getBuffer()->elapsedMilliseconds;
					if (streamingSessionPtr)
					{
						LOG(DEBUG) << LABELS{"proto"} << "server played=" << ((streamingSessionPtr->getFramesProvided() / samplingRate) * 1000);
					}
				}

				inline void onSTMt(CommandSTAT& commandSTAT, util::Timestamp receiveTimestamp)
				{
					auto timestampKey{commandSTAT.getBuffer()->serverTimestamp};

					// TODO: work in progress
					if (timestampKey)
					{
						auto sendTimestamp = timestampCache.find(timestampKey);
						if (sendTimestamp.has_value())
						{
							if (measuringLatency)
							{
								latency = (receiveTimestamp.getMicroSeconds() - sendTimestamp.value().getMicroSeconds()) / 2;

								LOG(DEBUG) << LABELS{"proto"} << "Client latency=" << latency;
							}
							else
							{
								timestampCache.erase(timestampKey);
							}

							if (timestampCache.size() > 10)
							{
								// TODO: should be removing the oldest
								timestampCache.erase(commandSTAT.getBuffer()->serverTimestamp);
							}
							else
							{
								ping();
							}
						}
						else
						{
							LOG(WARNING) << LABELS{"proto"} << "Invalid server timestamp data sent by a client (key received=" << commandSTAT.getBuffer()->serverTimestamp << ")";
						}
					}
					else
					{
						LOG(DEBUG) << LABELS{"proto"} << "Client sent ping request";
					}
				}

				template<typename CommandType>
				inline void send(CommandType&& command)
				{
					measuringLatency = false;

					// TODO: use local buffer and async API to prevent from interfering while measuring latency
					connection.get().write(command.getBuffer(), command.getSize());
				}

			private:
				std::reference_wrapper<ConnectionType>       connection;
				std::string                                  clientID;
				unsigned int                                 streamingPort{0};
				FormatSelection                              formatSelection;
				std::optional<unsigned int>                  gain;
				CommandHandlersMap                           commandHandlers;
				EventHandlersMap                             eventHandlers;
				bool                                         streaming{false};
				unsigned int                                 samplingRate{0};
				StreamingSessionType*                        streamingSessionPtr{nullptr};
				bool                                         connectedReceived{false};
				bool                                         responseReceived{false};
				util::ExpandableBuffer                       commandBuffer{std::size_t{0}, std::size_t{2048}};
				std::optional<CommandHELO>                   commandHELO{std::nullopt};
				util::TimestampCache                         timestampCache;
				bool                                         measuringLatency{false};
				unsigned int                                 latency{0};
		};
	}
}
