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

#include <functional>
#include <conwrap/ProcessorAsioProxy.hpp>

#include "slim/ContainerBase.hpp"


namespace slim
{
	class Server;

	class Session
	{
		public:
			Session(asio::ip::tcp::socket s, asio::ip::tcp::acceptor& acceptor, std::function<void(const std::error_code&)> openCallback)
			: nativeSocket{std::move(s)}
			{
				// TODO: validate result???
				acceptor.async_accept(
					nativeSocket,
					openCallback
				);
			}

			// using Rule Of Zero
			~Session()
			{
				if (!empty)
				{
					try
					{
						nativeSocket.shutdown(asio::socket_base::shutdown_both);
						nativeSocket.close();
					}
					catch(...) {}
				}
			}

			Session(const Session&) = delete;             // non-copyable
			Session& operator=(const Session&) = delete;  // non-assignable

			Session(Session&& rhs)
			: nativeSocket{std::move(rhs.nativeSocket)}
			{
				rhs.empty = true;
			}

			Session& operator=(Session&& rhs)
			{
				using std::swap;

				// any resources should be released for this object here because it will take over resources from rhs object
				if (!empty)
				{
					try
					{
						nativeSocket.shutdown(asio::socket_base::shutdown_both);
						nativeSocket.close();
					}
					catch(...) {}
				}
				rhs.empty = true;

				swap(nativeSocket, rhs.nativeSocket);

				return *this;
			}

		private:
			bool                  empty = false;
			asio::ip::tcp::socket nativeSocket;
	};
}
