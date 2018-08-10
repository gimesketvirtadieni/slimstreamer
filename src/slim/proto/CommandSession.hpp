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
				CommandSession(std::reference_wrapper<ConnectionType> co, std::string id, unsigned int p, FormatSelection f, std::optional<unsigned int> g, std::reference_wrapper<util::TimestampCache> tc)
				: connection{co}
				, clientID{id}
				, streamingPort{p}
				, formatSelection{f}
				, gain{g}
				, timestampCache{tc}
				, commandHandlers
				{
					{"DSCO", 0},
					{"HELO", [&](auto* buffer, auto size, auto timestamp) {return onHELO(buffer, size);}},
					{"RESP", 0},
					{"SETD", 0},
					{"STAT", [&](auto* buffer, auto size, auto timestamp) {return onSTAT(buffer, size, timestamp);}},
				}
				, eventHandlers
				{
					{"STMc", [&](auto& commandSTAT, auto timestamp) {onSTMc(commandSTAT);}},
					{"STMd", 0},
					{"STMf", 0},
					{"STMo", [&](auto& commandSTAT, auto timestamp) {onSTMo(commandSTAT);}},
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
					// adding data to the buffer
					commandBuffer.append(buffer, size);

					std::size_t processedSize{commandBuffer.size()};
					std::size_t keySize{4};
					if (processedSize > keySize)
					{
						// removing processed data from the buffer in an exception safe way
						::util::scope_guard guard = [&]
						{
							// TODO: this is the only use of shrinkLeft, consider alternative
							commandBuffer.shrinkLeft(processedSize);
						};

						std::string s{(char*)commandBuffer.data(), keySize};
						auto found{commandHandlers.find(s)};
						if (found != commandHandlers.end())
						{
							// making sure session HELO is the first command provided by a client
							if (!commandHELO.has_value() && (*found).first.compare("HELO"))
							{
								throw slim::Exception("Did not receive HELO command");
							}

							// if there is enough data to process this message
							auto b{commandBuffer.data()};
							auto s{commandBuffer.size()};
							if (Command<char>::isEnoughData(b, s))
							{
								if ((*found).second)
								{
									processedSize = (*found).second(b, s, timestamp);
								}
								else
								{
									// this is a dummy command processing routine
									LOG(INFO) << LABELS{"proto"} << (*found).first << " command received";
									processedSize = s;
								}
							}
						}
						else
						{
							LOG(INFO) << LABELS{"proto"} << "Unsupported SlimProto command received (header='" << s << "')";

							processedSize = commandBuffer.size();
						}
					}
				}

				inline void ping()
				{
					auto& nativeSocket{connection.get().getNativeSocket()};

					if (nativeSocket.is_open())
					{
						auto          command{CommandSTRM{CommandSelection::Time}};
						auto          buffer{command.getBuffer()};
						auto          size{command.getSize()};
						std::uint32_t newTimestampKey{0};
						int           result{0};
						std::size_t   sent{0};

						auto onError = [&]
						{
							// deleting a new timestamp entry if it was created
							if (newTimestampKey > 0)
							{
								timestampCache.get().erase(newTimestampKey);
							}
						};
						::util::scope_guard_failure onErrorGuard{onError};

						newTimestampKey = timestampCache.get().create();
						command.getBuffer()->data.replayGain = newTimestampKey;

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
							timestampCache.get().update(newTimestampKey, timestamp);

							if (pingTimestampKey > 0)
							{
								timestampCache.get().erase(pingTimestampKey);
							}
							pingTimestampKey = newTimestampKey;
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
				inline auto onHELO(unsigned char* buffer, std::size_t size)
				{
					std::size_t result{0};

					LOG(INFO) << LABELS{"proto"} << "HELO command received";

					// deserializing HELO command
					commandHELO = CommandHELO{buffer, size};
					result      = commandHELO.value().getSize();

					send(CommandSTRM{CommandSelection::Stop});
					send(CommandSETD{DeviceID::RequestName});
					send(CommandSETD{DeviceID::Squeezebox3});
					send(CommandAUDE{true, true});
					send(CommandAUDG{gain});

					if (streaming)
					{
						send(CommandSTRM{CommandSelection::Start, formatSelection, streamingPort, samplingRate, clientID});
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
						if ((*found).second)
						{
							(*found).second(commandSTAT, timestamp);
						}
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

				inline void onSTMo(CommandSTAT& commandSTAT)
				{
					LOG(WARNING) << LABELS{"proto"} << "xRUN notification sent by a client";
				}

				inline void onSTMs(CommandSTAT& commandSTAT)
				{
					// TODO: work in progress
					LOG(DEBUG) << LABELS{"proto"} << "1 STARTED=" << commandSTAT.getBuffer()->elapsedMilliseconds;
					if (streamingSessionPtr)
					{
						LOG(DEBUG) << LABELS{"proto"} << "2 STARTED=" << streamingSessionPtr->getSamplesProvided();
						LOG(DEBUG) << LABELS{"proto"} << "3 STARTED=" << streamingSessionPtr->getSamplesEncoded();
					}
				}

				inline void onSTMt(CommandSTAT& commandSTAT, util::Timestamp receiveTimestamp)
				{
					// TODO: work in progress
					auto sendTimestamp  = timestampCache.get().get(commandSTAT.getBuffer()->serverTimestamp).value();
					LOG(DEBUG) << LABELS{"proto"} << "round trip latency=" << receiveTimestamp.getMicroSeconds() - sendTimestamp.getMicroSeconds();
				}

				template<typename CommandType>
				inline void send(CommandType&& command)
				{
					connection.get().write(command.getBuffer(), command.getSize());
				}

			private:
				std::reference_wrapper<ConnectionType>       connection;
				std::string                                  clientID;
				unsigned int                                 streamingPort{0};
				FormatSelection                              formatSelection;
				std::optional<unsigned int>                  gain;
				std::reference_wrapper<util::TimestampCache> timestampCache;
				CommandHandlersMap                           commandHandlers;
				EventHandlersMap                             eventHandlers;
				bool                                         streaming{false};
				unsigned int                                 samplingRate{0};
				StreamingSessionType*                        streamingSessionPtr{nullptr};
				bool                                         connectedReceived{false};
				bool                                         responseReceived{false};
				util::ExpandableBuffer                       commandBuffer{std::size_t{0}, std::size_t{2048}};
				std::optional<CommandHELO>                   commandHELO{std::nullopt};
				std::uint32_t                                pingTimestampKey{0};
		};
	}
}
