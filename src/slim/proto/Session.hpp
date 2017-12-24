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

#include "slim/log/log.hpp"
#include "slim/proto/Command.hpp"


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType>
		class Session
		{
			public:
				Session(ConnectionType& c)
				: connection(c)
				{
					LOG(INFO) << "session created";

					auto command{Command{CommandSelection::STRM}};

					// writting to a socket
					// TODO: should be moved to Connection class
					auto& socket = connection.getNativeSocket();
					if (socket.is_open())
					{
						auto sent = socket.send(asio::buffer(command.getBuffer(), command.getSize()));
						LOG(INFO) << "sent=" << sent;
					}
				}

				// using Rule Of Zero
				~Session()
				{
					LOG(INFO) << "session deleted";
				}
				Session(const Session&) = delete;             // non-copyable
				Session& operator=(const Session&) = delete;  // non-assignable
				Session(Session&& rhs) = delete;              // non-movable
				Session& operator=(Session&& rhs) = delete;   // non-movable-assignable

				inline auto& getConnection()
				{
					return connection;
				}

				void onData(unsigned char* buffer, std::size_t receivedSize)
				{
				}

			private:
				ConnectionType& connection;
		};
	}
}
