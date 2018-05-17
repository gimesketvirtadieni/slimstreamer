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

#include "slim/conn/UDPCallbacks.hpp"
#include "slim/log/log.hpp"

// TODO: refactor
#define  BUFFER_SIZE 1


namespace slim
{
	namespace conn
	{
		template <typename ContainerType>
		class UDPServer
		{
			public:
				UDPServer(unsigned int p, UDPCallbacks<ContainerType> c)
				: port{p}
				, callbacks{std::move(c)}
				{
					LOG(DEBUG) << LABELS{"conn"} << "UDP server object was created (id=" << this << ")";
				}

			   ~UDPServer()
				{
					LOG(DEBUG) << LABELS{"conn"} << "UDP server object was deleted (id=" << this << ")";
				}

				UDPServer(const UDPServer&) = delete;             // non-copyable
				UDPServer& operator=(const UDPServer&) = delete;  // non-assignable
				UDPServer(UDPServer&& rhs) = delete;              // non-movable
				UDPServer& operator=(UDPServer&& rhs) = delete;   // non-movable-assinable

				void setProcessorProxy(conwrap::ProcessorAsioProxy<ContainerType>* p)
				{
					processorProxyPtr = p;
				}

				void start()
				{
					LOG(INFO) << LABELS{"conn"} << "Starting new UDP server (id=" << this << ", port=" << port << ")...";

					// opening UDP socket
					openSocket();

					// calling start callback and changing state of this object to 'started'
					callbacks.getStartCallback()(*this);
					started = true;

					LOG(INFO) << LABELS{"conn"} << "UDP server was started (id=" << this << ")";
				}

				void stop()
				{
					LOG(INFO) << LABELS{"conn"} << "Stopping UDP server (id=" << this << ", port=" << port << ")...";

					// closing UDP socket
					closeSocket();

					// calling stop callback and changing state of this object to '!started'
					callbacks.getStopCallback()(*this);
					started = false;

					LOG(INFO) << LABELS{"conn"} << "UDP server was stopped (id=" << this << ", port=" << port << ")";
				}

			protected:
				void closeSocket()
				{
					// disposing acceptor to prevent from new incomming requests
					if (socketPtr)
					{
						socketPtr->cancel();
						socketPtr->close();

						LOG(INFO) << LABELS{"conn"} << "UDP socket was closed (id=" << socketPtr.get() << ", port=" << port << ")";

						// acceptor is not captured by handlers so it is safe to delete it
						socketPtr.reset();
					}
				}

				void onData(const std::error_code error, const std::size_t size)
				{
					if (!error)
					{
						// processing received data
						callbacks.getDataCallback()(*this, buffer, size);

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
					if (!socketPtr)
					{
						socketPtr = std::make_unique<asio::ip::udp::socket>(
							*processorProxyPtr->getDispatcher(),
							asio::ip::udp::endpoint(
								asio::ip::udp::v4(),
								port
							)
						);
						LOG(INFO) << LABELS{"conn"} << "UDP socket was opened (id=" << socketPtr.get() << ", port=" << port << ")";
					}

					// start receiving data
					receiveData();
				}

				void receiveData()
				{
					if (socketPtr)
					{
						socketPtr->async_receive_from(asio::buffer(buffer, BUFFER_SIZE), endpoint, [=](auto error, auto transferred)
						{
							processorProxyPtr->wrap([=]
							{
								onData(error, transferred);
							})();
						});
					}
				}

			private:
				unsigned int                                port;
				UDPCallbacks<ContainerType>                 callbacks;
				bool                                        started{false};
				conwrap::ProcessorAsioProxy<ContainerType>* processorProxyPtr;
				std::unique_ptr<asio::ip::udp::socket>      socketPtr;
				asio::ip::udp::endpoint                     endpoint;
				unsigned char                               buffer[BUFFER_SIZE];
		};
	}
}
