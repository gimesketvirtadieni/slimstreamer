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
#include <cstddef>  // std::size_t
#include <slim/conn/CallbacksBase.hpp>
#include <slim/conn/Connection.hpp>
#include <memory>
#include <vector>

#include "slim/conn/Callbacks.hpp"
#include "slim/conn/Connection.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	namespace conn
	{
		template <typename ContainerType>
		class Server
		{
			public:
				Server(unsigned int p, unsigned int m, Callbacks<ContainerType> c)
				: port{p}
				, maxConnections{m}
				, callbacks
				{
					std::move(c.startCallback),
					callbacks.openCallback = [&, openCallback = std::move(c.openCallback)](auto& connection)
					{
						if (openCallback)
						{
							openCallback(connection);
						}

						// registering a new connection if capacity allows so new requests can be accepted
						if (connections.size() < maxConnections)
						{
							addConnection();
						}
						else
						{
							LOG(DEBUG) << LABELS{"slim"} << "Limit of active connections was reached (id=" << this << ", connections=" << connections.size() << " max=" << maxConnections << ")";
							stopAcceptor();
						}
					},
					std::move(c.dataCallback),
					std::move(c.closeCallback),
					callbacks.stopCallback = [&, stopCallback = std::move(c.stopCallback)](auto& connection)
					{
						if (stopCallback)
						{
							stopCallback(connection);
						}

						// connection cannot be removed at this moment as this method is called by the connection which is being removed
						processorProxyPtr->process([&]
						{
							removeConnection(connection);

							// starting acceptor if required and adding new connection to accept client requests
							if (started && !acceptorPtr)
							{
								startAcceptor();
								addConnection();
							}
						});
					}
				}
				, started{false} {}

				// using Rule Of Zero
				~Server()
				{
					LOG(INFO) << "server deleted";
				}
				Server(const Server&) = delete;             // non-copyable
				Server& operator=(const Server&) = delete;  // non-assignable
				Server(Server&& rhs) = delete;              // non-movable
				Server& operator=(Server&& rhs) = delete;   // non-movable-assinable

				void setProcessorProxy(conwrap::ProcessorAsioProxy<ContainerType>* p)
				{
					processorProxyPtr = p;
				}

				void start()
				{
					LOG(INFO) << LABELS{"slim"} << "Starting new server (id=" << this << ", port=" << port << ", max connections=" << maxConnections << ")...";

					// start accepting new requests
					startAcceptor();
					addConnection();

					started = true;
					LOG(INFO) << LABELS{"slim"} << "Server was started (id=" << this << ")";
				}

				void stop()
				{
					LOG(INFO) << LABELS{"slim"} << "Stopping server...";
					started = false;

					// stop accepting any new connections
					stopAcceptor();

					// closing active connections; connection will be removed by onStop callback
					for (auto& connectionPtr : connections)
					{
						connectionPtr->stop();
					}

					LOG(INFO) << LABELS{"slim"} << "Server was stopped (id=" << this << ")";
				}

			protected:
				void addConnection()
				{
					LOG(DEBUG) << LABELS{"slim"} << "Adding new connection (connections=" << connections.size() << ")...";

					// creating new connection
					auto connectionPtr{std::make_unique<Connection<ContainerType>>(processorProxyPtr, callbacks)};

					// start accepting connection
					connectionPtr->start(*acceptorPtr);

					// adding connection to the connection vector
					connections.push_back(std::move(connectionPtr));

					LOG(DEBUG) << LABELS{"slim"} << "New connection was added (id=" << this << ", connections=" << connections.size() << ")";
				}

				void removeConnection(Connection<ContainerType>& connection)
				{
					LOG(DEBUG) << LABELS{"slim"} << "Removing connection (id=" << &connection << ", connections=" << connections.size() << ")...";

					// removing connection from the vector
					connections.erase(
						std::remove_if(
							connections.begin(),
							connections.end(),
							[&](auto& connectionPtr) -> bool
							{
								return connectionPtr.get() == &connection;
							}
						),
						connections.end()
					);

					LOG(DEBUG) << LABELS{"slim"} << "Connection was removed (connections=" << connections.size() << ")";
				}

				void startAcceptor()
				{
					// creating an acceptor if required
					if (!acceptorPtr)
					{
						LOG(DEBUG) << LABELS{"slim"} << "Starting acceptor...";

						acceptorPtr = std::make_unique<asio::ip::tcp::acceptor>(
							*processorProxyPtr->getDispatcher(),
							asio::ip::tcp::endpoint(
								asio::ip::tcp::v4(),
								port
							)
						);

						LOG(DEBUG) << LABELS{"slim"} << "Acceptor was started (id=" << acceptorPtr.get() << ")";
					}
				}

				void stopAcceptor()
				{
					// disposing acceptor to prevent from new incomming requests
					if (acceptorPtr) {
						LOG(DEBUG) << LABELS{"slim"} << "Stopping acceptor (id=" << acceptorPtr.get() << ")...";

						acceptorPtr->cancel();
						acceptorPtr->close();

						// acceptor is not captured by handlers so it is safe to delete it
						acceptorPtr.reset();

						LOG(DEBUG) << LABELS{"slim"} << "Acceptor was stopped (id=" << acceptorPtr.get() << ")";
					}
				}

			private:
				unsigned int                                            port;
				unsigned int                                            maxConnections;
				Callbacks<ContainerType>                                callbacks;
				bool                                                    started;
				conwrap::ProcessorAsioProxy<ContainerType>*             processorProxyPtr;
				std::unique_ptr<asio::ip::tcp::acceptor>                acceptorPtr;
				std::vector<std::unique_ptr<Connection<ContainerType>>> connections;
		};
	}
}
