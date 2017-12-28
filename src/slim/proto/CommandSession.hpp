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

#include <cstdint>  // std::u..._t types

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

					send(CommandSTRM{CommandSelection::Stop});
					send(CommandSETD{DeviceID::RequestName});
					send(CommandSETD{DeviceID::Squeezebox3});
					send(CommandAUDE{true, true});
					send(CommandAUDG{});

					send(CommandSTRM{CommandSelection::Start});
				}

				~CommandSession()
				{
					LOG(INFO) << "SlimProto session deleted";
				}
				CommandSession(const CommandSession&) = delete;             // non-copyable
				CommandSession& operator=(const CommandSession&) = delete;  // non-assignable
				CommandSession(CommandSession&& rhs) = delete;              // non-movable
				CommandSession& operator=(CommandSession&& rhs) = delete;   // non-movable-assignable

				inline auto& getConnection()
				{
					return connection;
				}

				void onData(unsigned char* buffer, std::size_t receivedSize)
				{
				}

			protected:
				// TODO: should be moved to Connection class???
				template<typename CommandType>
				void send(CommandType command)
				{
					auto& socket = connection.getNativeSocket();
					if (socket.is_open())
					{
						// preparing command size in indianess-independant way
						auto size = command.getSize();
						char sizeBuffer[2];
						sizeBuffer[0] = 255 & (size >> 8);
						sizeBuffer[1] = 255 & size;

						// sending command size
						socket.send(asio::buffer(sizeBuffer, sizeof(sizeBuffer)));

						// sending command data
						socket.send(asio::buffer(command.getBuffer(), command.getSize()));
					}
				}

			private:
				ConnectionType& connection;
		};
	}
}
