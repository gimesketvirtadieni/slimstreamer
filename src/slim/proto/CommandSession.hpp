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


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType, typename EncoderType>
		class CommandSession
		{
			using CommandHandlersMap   = std::unordered_map<std::string, std::function<std::size_t(unsigned char*, std::size_t)>>;
			using EventHandlersMap     = std::unordered_map<std::string, std::function<void()>>;
			using TimePoint            = std::chrono::time_point<std::chrono::steady_clock>;
			using StreamingSessionType = StreamingSession<ConnectionType, EncoderType>;

			public:
				CommandSession(std::reference_wrapper<ConnectionType> co, std::string id, std::optional<unsigned int> g)
				: connection{co}
				, clientID{id}
				, gain{g}
				, commandHandlers
				{
					{"DSCO", 0},
					{"HELO", [&](auto* buffer, auto size) {return onHELO(buffer, size);}},
					{"RESP", 0},
					{"SETD", 0},
					{"STAT", [&](auto* buffer, auto size) {return onSTAT(buffer, size);}},
				}
				, eventHandlers
				{
					{"STMc", [&] {onSTMc();}},
					{"STMf", 0},
					{"STMo", 0},
					{"STMt", 0},
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

				inline auto isConnectedReceived()
				{
					return connectedReceived;
				}

				inline auto isResponseReceived()
				{
					return responseReceived;
				}

				inline void onRequest(unsigned char* buffer, std::size_t size)
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
									processedSize = (*found).second(b, s);
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
							LOG(DEBUG) << LABELS{"proto"} << "Unsupported SlimProto command received (header='" << s << "')";

							processedSize = commandBuffer.size();
						}
					}
				}

				inline void ping()
				{
					auto  command{CommandSTRM{CommandSelection::Time}};
					auto  buffer{command.getBuffer()};
					auto  size{command.getSize()};
					auto& nativeSocket{connection.get().getNativeSocket()};

					if (nativeSocket.is_open())
					{
						auto asioBuffer{asio::buffer(buffer, size)};

						// saving previous time; will be restored in case of an error
						auto tmp{lastPingAt};

						// saving ping time as close as possible to the moment of sending data out
						lastPingAt = std::chrono::steady_clock::now();

						// sending first part
						auto sent{nativeSocket.send(asioBuffer)};

						// sending remainder (it almost never happens)
						while (sent > 0 && sent < size)
						{
							auto s{nativeSocket.send(asio::buffer(&buffer[sent], size - sent))};
							if (s > 0)
							{
								s += sent;
							}
							sent = s;
						}

						// restoring the last ping time value in case of an error
						lastPingAt = tmp;
					}
				}

				inline void setStreamingSession(StreamingSessionType* s)
				{
					streamingSessionPtr = s;
					if (!s)
					{
						connectedReceived = false;
						responseReceived  = false;
					}
				}

				inline void stream(unsigned int p, unsigned int r)
				{
					// TODO: it's never set back to false
					streaming     = true;
					streamingPort = p;
					samplingRate  = r;

					// if handshake commands were sent
					if (commandHELO.has_value())
					{
						send(CommandSTRM{CommandSelection::Start, streamingPort, samplingRate, clientID});
					}
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
						send(CommandSTRM{CommandSelection::Start, streamingPort, samplingRate, clientID});
					}

					return result;
				}

				inline auto onSTAT(unsigned char* buffer, std::size_t size)
				{
					std::size_t result{0};

					// deserializing STAT command
					auto commandSTAT{CommandSTAT{buffer, size}};
					result = commandSTAT.getSize();

					auto event{commandSTAT.getEvent()};
					auto found{eventHandlers.find(event)};
					if (found != eventHandlers.end())
					{
						LOG(DEBUG) << LABELS{"proto"} << event << " event received";
						if ((*found).second)
						{
							(*found).second();
						}
					}
					else
					{
						LOG(DEBUG) << LABELS{"proto"} << "Unsupported STAT event received: " << event;
					}

					return result;
				}

				inline void onSTMc()
				{
					connectedReceived = true;
				}

				template<typename CommandType>
				inline void send(CommandType command)
				{
					connection.get().write(command.getBuffer(), command.getSize());
				}

			private:
				std::reference_wrapper<ConnectionType> connection;
				std::string                            clientID;
				std::optional<unsigned int>            gain;
				CommandHandlersMap                     commandHandlers;
				EventHandlersMap                       eventHandlers;
				bool                                   streaming{false};
				unsigned int                           streamingPort{0};
				unsigned int                           samplingRate{0};
				StreamingSessionType*                  streamingSessionPtr{nullptr};
				bool                                   connectedReceived{false};
				bool                                   responseReceived{false};
				util::ExpandableBuffer                 commandBuffer{std::size_t{0}, std::size_t{2048}};
				std::optional<CommandHELO>             commandHELO{std::nullopt};
				std::optional<TimePoint>               lastPingAt{std::nullopt};
		};
	}
}
