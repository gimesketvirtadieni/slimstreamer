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
#include <memory>
#include <optional>

#include "slim/conn/udp/CallbacksBase.hpp"
#include "slim/log/log.hpp"
#include "slim/util/AsyncWriter.hpp"

// TODO: parametrize
#define  BUFFER_SIZE 1


namespace slim
{
	namespace conn
	{
		namespace udp
		{
			template <typename ContainerType>
			class Server : public util::AsyncWriter
			{
				public:
					Server(unsigned int p, std::unique_ptr<CallbacksBase<Server<ContainerType>>> c)
					: port{p}
					, callbacksPtr{std::move(c)}
					{
						LOG(DEBUG) << LABELS{"conn"} << "UDP server object was created (id=" << this << ")";
					}

					virtual ~Server()
					{
						LOG(DEBUG) << LABELS{"conn"} << "UDP server object was deleted (id=" << this << ")";
					}

					Server(const Server&) = delete;             // non-copyable
					Server& operator=(const Server&) = delete;  // non-assignable
					Server(Server&& rhs) = delete;              // non-movable
					Server& operator=(Server&& rhs) = delete;   // non-movable-assinable

					auto& getNativePeerEndpoint()
					{
						return peerEndpoint;
					}

					auto& getNativeSocket()
					{
						return nativeSocket;
					}

					virtual void rewind(const std::streampos pos) override {}

					void setProcessorProxy(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>>* p)
					{
						processorProxyPtr = p;
					}

					void start()
					{
						LOG(INFO) << LABELS{"conn"} << "Starting new UDP server (id=" << this << ", port=" << port << ")...";

						// opening UDP socket
						openSocket();

						// calling start callback and changing state of this object to 'started'
						try
						{
							callbacksPtr->getStartCallback()(*this);
						}
						catch (const std::exception& e)
						{
							LOG(ERROR) << LABELS{"conn"} << "Error while invoking start callback (id=" << this << ", error=" << e.what() << ")";
						}
						started = true;

						LOG(INFO) << LABELS{"conn"} << "UDP server was started (id=" << this << ")";
					}

					void stop()
					{
						LOG(INFO) << LABELS{"conn"} << "Stopping UDP server (id=" << this << ", port=" << port << ")...";

						// closing UDP socket
						closeSocket();

						// resetting peer's UDP endpoint
						peerEndpoint = std::nullopt;

						// calling stop callback and changing state of this object to '!started'
						try
						{
							callbacksPtr->getStopCallback()(*this);
						}
						catch (const std::exception& e)
						{
							LOG(ERROR) << LABELS{"conn"} << "Error while invoking stop callback (id=" << this << ", error=" << e.what() << ")";
						}
						started = false;

						LOG(INFO) << LABELS{"conn"} << "UDP server was stopped (id=" << this << ", port=" << port << ")";
					}

					// including write overloads
					using AsyncWriter::write;

					virtual std::size_t write(const void* data, const std::size_t size) override
					{
						std::size_t result{0};

						if (nativeSocket.has_value() && nativeSocket.value().is_open())
						{
							if (peerEndpoint.has_value())
							try
							{
								result = nativeSocket.value().send_to(std::experimental::net::const_buffer(data, size), peerEndpoint.value());
							}
							catch(const std::system_error& e)
							{
								LOG(ERROR) << LABELS{"conn"} << "Could not send data due to an error (id=" << this << ", error=" << e.what() << ")";
							}
							else
							{
								LOG(WARNING) << LABELS{"conn"} << "Data was not sent due to missing peer's endpoint (id=" << this << ")";
							}
						}
						else
						{
							LOG(WARNING) << LABELS{"conn"} << "Data was not sent due to closed socket (id=" << this << ")";
						}

						return result;
					}

					// including writeAsync overloads
					using AsyncWriter::writeAsync;

					virtual void writeAsync(const void* data, const std::size_t size, util::WriteCallback callback = [](auto, auto) {}) override
					{

					}

				protected:
					void closeSocket()
					{
						// disposing acceptor to prevent from new incomming requests
						if (nativeSocket.has_value())
						{
							nativeSocket.value().cancel();
							nativeSocket.value().close();

							LOG(INFO) << LABELS{"conn"} << "UDP socket was closed (id=" << &nativeSocket.value() << ", port=" << port << ")";

							// acceptor is not captured by handlers so it is safe to delete it
							nativeSocket = std::nullopt;
						}
					}

					void onData(const std::error_code error, const std::size_t size)
					{
						if (!error)
						{
							// processing received data
							try
							{
								callbacksPtr->getDataCallback()(*this, buffer, size);
							}
							catch (const std::exception& e)
							{
								LOG(ERROR) << LABELS{"conn"} << "Error while invoking data callback (id=" << this << ", error=" << e.what() << ")";
							}

							// keep receiving data
							receiveData();
						}
						else
						{
							LOG(WARNING) << LABELS{"conn"} << "Error while receiving data: " << error.message();
						}
					}

					void openSocket()
					{
						// creating a socket if required
						if (!nativeSocket.has_value())
						{
							nativeSocket = std::experimental::net::ip::udp::socket
							{
								processorProxyPtr->getDispatcher(),
								std::experimental::net::ip::udp::endpoint(
									std::experimental::net::ip::udp::v4(),
									port
								)
							};
							LOG(INFO) << LABELS{"conn"} << "UDP socket was opened (id=" << &nativeSocket.value() << ", port=" << port << ")";
						}

						// allocating new endpoint required for UDP (peer's side)
						peerEndpoint = std::experimental::net::ip::udp::endpoint();

						// start receiving data
						receiveData();
					}

					void receiveData()
					{
						if (nativeSocket.has_value())
						{
							nativeSocket.value().async_receive_from(std::experimental::net::buffer(buffer, BUFFER_SIZE), peerEndpoint.value(), [=](auto error, auto transferred)
							{
								onData(error, transferred);
							});
						}
					}

				private:
					unsigned int                                              port;
					std::unique_ptr<CallbacksBase<Server>>                    callbacksPtr;
					bool                                                      started{false};
					conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>>* processorProxyPtr;
					std::optional<std::experimental::net::ip::udp::socket>    nativeSocket{std::nullopt};
					std::optional<std::experimental::net::ip::udp::endpoint>  peerEndpoint{std::nullopt};
					unsigned char                                             buffer[BUFFER_SIZE];
			};
		}
	}
}
