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

				void onRequest(unsigned char* buffer, std::size_t receivedSize)
				{
				}

			// TODO: refactor usage
			//protected:
				template<typename CommandType>
				void send(CommandType command)
				{
					// TODO: introduce buffer wrapper so it can be passed to a stream; then in can be moved to a Command class
					for (std::size_t i = 0; i < command.getSize();)
					{
						i += connection.send(command.getBuffer() + i, command.getSize() - i);
					}
				}

			private:
				ConnectionType& connection;
		};
	}
}
