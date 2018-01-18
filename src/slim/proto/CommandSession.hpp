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
#include <sstream>  // std::stringstream
#include <string>

#include "slim/log/log.hpp"
#include "slim/proto/CommandAUDE.hpp"
#include "slim/proto/CommandAUDG.hpp"
#include "slim/proto/CommandSETD.hpp"
#include "slim/proto/CommandSTRM.hpp"


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType>
		class CommandSession
		{
			public:
				CommandSession(ConnectionType& c)
				: connection(c)
				{
					LOG(INFO) << "SlimProto session created";

					// TODO: get from parameter
					std::stringstream ss;
			        ss << static_cast<const void*>(this);
			        clientID = ss.str();

			        send(CommandSTRM{CommandSelection::Stop});
					send(CommandSETD{DeviceID::RequestName});
					send(CommandSETD{DeviceID::Squeezebox3});
					send(CommandAUDE{true, true});
					send(CommandAUDG{});
				}

				~CommandSession()
				{
					LOG(INFO) << "SlimProto session deleted";
				}

				CommandSession(const CommandSession&) = delete;             // non-copyable
				CommandSession& operator=(const CommandSession&) = delete;  // non-assignable
				CommandSession(CommandSession&& rhs) = delete;              // non-movable
				CommandSession& operator=(CommandSession&& rhs) = delete;   // non-movable-assignable

				inline auto getClientID()
				{
					return clientID;
				}

				inline void onRequest(unsigned char* buffer, std::size_t receivedSize)
				{
				}

				inline void ping()
				{
					// TODO: introduce buffer wrapper so it can be passed to a stream; then in can be moved to a Command class
					auto  command{CommandSTRM{CommandSelection::Time}};
					auto  buffer{command.getBuffer()};
					auto  size{command.getSize()};
					auto& nativeSocket{connection.getNativeSocket()};

					if (nativeSocket.is_open())
					{
						auto asioBuffer{asio::buffer(buffer, size)};

						// saving ping time as close as possible before sending data out
						lastPingTime = std::chrono::steady_clock::now();

						// sending first part
						auto sent{nativeSocket.send(asioBuffer)};

						// sending remainder (this almost never happens)
						while (sent < size)
						{
							sent += nativeSocket.send(asio::buffer(&buffer[sent], size - sent));
						}
					}
				}

			// TODO: refactor usage
			//protected:
				template<typename CommandType>
				inline void send(CommandType command)
				{
					// TODO: introduce buffer wrapper so it can be passed to a stream; then in can be moved to a Command class
					for (std::size_t i = 0; i < command.getSize();)
					{
						i += connection.send(command.getBuffer() + i, command.getSize() - i);
					}
				}

			private:
				ConnectionType&                                    connection;
				std::string                                        clientID;
				// TODO: initialization ???
				std::chrono::time_point<std::chrono::steady_clock> lastPingTime;
		};
	}
}
