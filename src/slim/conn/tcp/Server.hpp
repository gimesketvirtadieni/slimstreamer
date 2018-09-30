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

#include <experimental/net>
#include <conwrap2/ProcessorProxy.hpp>
#include <cstddef>  // std::size_t
#include <memory>
#include <vector>

#include "slim/conn/tcp/Callbacks.hpp"
#include "slim/conn/tcp/Connection.hpp"
#include "slim/log/log.hpp"
#include "slim/util/Timestamp.hpp"


namespace slim
{
	namespace conn
	{
		namespace tcp
		{
			template <typename ContainerType>
			class Server
			{
				public:
					Server(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pp, unsigned int po, unsigned int ma, std::unique_ptr<Callbacks<ContainerType>> ca)
					: processorProxy{pp}
					, port{po}
					, maxConnections{ma}
					, callbacksPtr{std::make_unique<Callbacks<ContainerType>>()}
					, started{false}
					{
						callbacksPtr->setStartCallback(std::move([&, startCallback = std::move(ca->getStartCallback())](auto& connection)
						{
							try
							{
								startCallback(connection);
							}
							catch (const std::exception& e)
							{
								LOG(ERROR) << LABELS{"conn"} << "Error while invoking 'on start' callback (id=" << this << ", error=" << e.what() << ")";
							}
						}));
						callbacksPtr->setOpenCallback(std::move([&, openCallback = std::move(ca->getOpenCallback())](auto& connection)
						{
							try
							{
								openCallback(connection);
							}
							catch (const std::exception& e)
							{
								LOG(ERROR) << LABELS{"conn"} << "Error while invoking 'on open' callback (id=" << this << ", error=" << e.what() << ")";
							}

							// for whatever / unknown reason std::count_if returns signed!!! result, hence a type cast is required
							auto foundTotal{std::count_if(connections.begin(), connections.end(), [&](auto& connectionPtr)
							{
								return connectionPtr->isOpen();
							})};

							// registering a new connection if capacity allows so new requests can be accepted
							if (foundTotal <= 0 || static_cast<unsigned int>(foundTotal) < maxConnections)
							{
								addConnection();
							}
							else
							{
								LOG(WARNING) << LABELS{"conn"} << "Limit of active connections was reached (id=" << this << ", connections=" << connections.size() << " max=" << maxConnections << ")";
								stopAcceptor();
							}
						}));
						callbacksPtr->setDataCallback(std::move([&, dataCallback = std::move(ca->getDataCallback())](auto& connection, unsigned char* buffer, const std::size_t size, const util::Timestamp timestamp)
						{
							try
							{
								dataCallback(connection, buffer, size, timestamp);
							}
							catch (const std::exception& e)
							{
								LOG(ERROR) << LABELS{"conn"} << "Error while invoking 'on data' callback (id=" << this << ", error=" << e.what() << ")";
							}
						}));
						callbacksPtr->setCloseCallback(std::move([&, closeCallback = std::move(ca->getCloseCallback())](auto& connection)
						{
							try
							{
								closeCallback(connection);
							}
							catch (const std::exception& e)
							{
								LOG(ERROR) << LABELS{"conn"} << "Error while invoking 'on close' callback (id=" << this << ", error=" << e.what() << ")";
							}
						}));
						callbacksPtr->setStopCallback(std::move([&, stopCallback = std::move(ca->getStopCallback())](auto& connection)
						{
							try
							{
								stopCallback(connection);
							}
							catch (const std::exception& e)
							{
								LOG(ERROR) << LABELS{"conn"} << "Error while invoking 'on stop' callback (id=" << this << ", error=" << e.what() << ")";
							}

							// connection cannot be removed at this moment as this method is called by the connection which is being removed
							processorProxy.process([&]
							{
								removeConnection(connection);

								// starting acceptor if required and adding new connection to accept client requests
								if (started && !acceptorPtr)
								{
									startAcceptor();
									addConnection();
								}
							});
						}));

						LOG(DEBUG) << LABELS{"conn"} << "TCP server object was created (id=" << this << ")";
					}

				   ~Server()
					{
						LOG(DEBUG) << LABELS{"conn"} << "TCP server object was deleted (id=" << this << ")";
					}

					Server(const Server&) = delete;             // non-copyable
					Server& operator=(const Server&) = delete;  // non-assignable
					Server(Server&& rhs) = delete;              // non-movable
					Server& operator=(Server&& rhs) = delete;   // non-movable-assinable

					void start()
					{
						LOG(INFO) << LABELS{"conn"} << "Starting TCP new server (id=" << this << ", port=" << port << ", max connections=" << maxConnections << ")...";

						// start accepting new requests
						startAcceptor();
						addConnection();

						started = true;
						LOG(INFO) << LABELS{"conn"} << "TCP server was started (id=" << this << ")";
					}

					void stop()
					{
						LOG(INFO) << LABELS{"conn"} << "Stopping TCP server...";
						started = false;

						// stop accepting any new connections
						stopAcceptor();

						// closing active connections; connection will be removed by onStop callback
						for (auto& connectionPtr : connections)
						{
							connectionPtr->stop();
						}

						LOG(INFO) << LABELS{"conn"} << "TCP server was stopped (id=" << this << ", port=" << port << ", max connections=" << maxConnections << ")";
					}

				protected:
					auto& addConnection()
					{
						// creating new connection
						auto connectionPtr{std::make_unique<Connection<ContainerType>>(processorProxy, *callbacksPtr.get())};

						// start accepting connection
						connectionPtr->start(*acceptorPtr);

						// required to return to the caller
						auto* c = connectionPtr.get();

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

					void startAcceptor()
					{
						// creating an acceptor if required
						if (!acceptorPtr)
						{
							acceptorPtr = std::make_unique<std::experimental::net::ip::tcp::acceptor>(
								processorProxy.getDispatcher(),
								std::experimental::net::ip::tcp::endpoint(
									std::experimental::net::ip::tcp::v4(),
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

							LOG(INFO) << LABELS{"conn"} << "Acceptor was stopped (id=" << acceptorPtr.get() << ", port=" << port << ")";

							// acceptor is not captured by handlers so it is safe to delete it
							acceptorPtr.reset();
						}
					}

				private:
					conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>>   processorProxy;
					unsigned int                                               port;
					unsigned int                                               maxConnections;
					std::unique_ptr<Callbacks<ContainerType>>                  callbacksPtr;
					bool                                                       started;
					std::unique_ptr<std::experimental::net::ip::tcp::acceptor> acceptorPtr;
					std::vector<std::unique_ptr<Connection<ContainerType>>>    connections;
			};
		}
	}
}
