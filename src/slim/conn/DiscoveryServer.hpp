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

#include "slim/log/log.hpp"


namespace slim
{
	namespace conn
	{
		template <typename ContainerType>
		class DiscoveryServer
		{
			public:
				DiscoveryServer(unsigned int p)
				: port{p}
				{
					LOG(DEBUG) << LABELS{"conn"} << "DiscoveryServer object was created (id=" << this << ")";
				}

			   ~DiscoveryServer()
				{
					LOG(DEBUG) << LABELS{"conn"} << "DiscoveryServer object was deleted (id=" << this << ")";
				}

				DiscoveryServer(const DiscoveryServer&) = delete;             // non-copyable
				DiscoveryServer& operator=(const DiscoveryServer&) = delete;  // non-assignable
				DiscoveryServer(DiscoveryServer&& rhs) = delete;              // non-movable
				DiscoveryServer& operator=(DiscoveryServer&& rhs) = delete;   // non-movable-assinable

				void setProcessorProxy(conwrap::ProcessorAsioProxy<ContainerType>* p)
				{
					processorProxyPtr = p;
				}

				void start()
				{
					LOG(INFO) << LABELS{"conn"} << "Starting new discovery server (id=" << this << ", port=" << port << ")...";

					// start accepting new requests
					//startAcceptor();
					//addConnection();

					started = true;
					LOG(INFO) << LABELS{"conn"} << "Discovery server was started (id=" << this << ")";
				}

				void stop()
				{
					LOG(INFO) << LABELS{"conn"} << "Stopping discovery server (id=" << this << ", port=" << port << ")...";
					started = false;

					// stop accepting any new connections
					//stopAcceptor();

					// closing active connections; connection will be removed by onStop callback
					//for (auto& connectionPtr : connections)
					//{
					//	connectionPtr->stop();
					//}

					LOG(INFO) << LABELS{"conn"} << "Discovery server was stopped (id=" << this << ", port=" << port << ")";
				}

			protected:
/*
				auto& addConnection()
				{
					// creating new connection
					auto connectionPtr{std::make_unique<Connection<ContainerType>>(processorProxyPtr, callbacks)};

					// start accepting connection
					connectionPtr->start(*acceptorPtr);

					// required to return to the caller
					auto c = connectionPtr.get();

					// adding connection to the connection vector
					connections.push_back(std::move(connectionPtr));

					LOG(INFO) << LABELS{"conn"} << "New connection was added (id=" << c << ", connections=" << connections.size() << ")";

					return *c;
				}

				void removeConnection(Connection<ContainerType>& connection)
				{
					// removing connection from the vector
					connections.erase(std::remove_if(connections.begin(), connections.end(), [&](auto& connectionPtr) -> bool
					{
						return connectionPtr.get() == &connection;
					}), connections.end());

					LOG(INFO) << LABELS{"conn"} << "Connection was removed (connections=" << connections.size() << ")";
				}
*/
				void startAcceptor()
				{
					// creating an acceptor if required
					if (!acceptorPtr)
					{
						acceptorPtr = std::make_unique<asio::ip::tcp::acceptor>(
							*processorProxyPtr->getDispatcher(),
							asio::ip::tcp::endpoint(
								asio::ip::tcp::v4(),
								port
							)
						);

						LOG(INFO) << LABELS{"conn"} << "Acceptor was started (id=" << acceptorPtr.get() << ", port=" << port << ")";
					}
				}

				void stopAcceptor()
				{
					// disposing acceptor to prevent from new incomming requests
					if (acceptorPtr) {
						acceptorPtr->cancel();
						acceptorPtr->close();

						// acceptor is not captured by handlers so it is safe to delete it
						acceptorPtr.reset();

						LOG(INFO) << LABELS{"conn"} << "Acceptor was stopped (id=" << acceptorPtr.get() << ", port=" << port << ")";
					}
				}

			private:
				unsigned int                                            port;
				bool                                                    started{false};
				conwrap::ProcessorAsioProxy<ContainerType>*             processorProxyPtr;
				std::unique_ptr<asio::ip::tcp::acceptor>                acceptorPtr;
		};
	}
}
