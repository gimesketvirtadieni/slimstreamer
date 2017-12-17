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

#include <asio.hpp>
#include <conwrap/ProcessorAsioProxy.hpp>
#include <memory>
#include <vector>

#include "slim/ContainerBase.hpp"
#include "slim/Session.hpp"


namespace slim
{
	class Server
	{
		public:
			Server(unsigned int p, unsigned int m)
			: port{p}
			, maxSessions{m}
			, started(false) {}

			// using Rule Of Zero
			~Server()
			{
				LOG(INFO) << "server deleted";
			}
			Server(const Server&) = delete;             // non-copyable
			Server& operator=(const Server&) = delete;  // non-assignable
			Server(Server&& rhs) = delete;              // non-movable
			Server& operator=(Server&& rhs) = delete;   // non-movable-assinable

			void setProcessorProxy(conwrap::ProcessorAsioProxy<ContainerBase>* p);
			void start();
			void stop();

		protected:
			void addSession();
			void removeSession(Session& session);
			void startAcceptor();
			void stopAcceptor();

		private:
			unsigned int                                port;
			unsigned int                                maxSessions;
			bool                                        started;
			conwrap::ProcessorAsioProxy<ContainerBase>* processorProxyPtr;
			std::unique_ptr<asio::ip::tcp::acceptor>    acceptorPtr;
			std::vector<std::unique_ptr<Session>>       sessions;
	};
}
