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

#include <conwrap/ProcessorAsioProxy.hpp>
#include <cstddef>  // std::size_t
#include <functional>
#include <iostream>
#include <memory>

#include "slim/conn/CallbacksBase.hpp"
#include "slim/log/log.hpp"
#include "slim/util/OutputStreamCallback.hpp"


namespace slim
{
	namespace conn
	{
		template <typename ContainerType>
		class Connection
		{
			using SocketStreamCallback = util::OutputStreamCallback<std::function<std::streamsize(const char*, std::streamsize)>>;

			public:
				Connection(conwrap::ProcessorAsioProxy<ContainerType>* p, CallbacksBase<Connection<ContainerType>>& c)
				: processorProxyPtr{p}
				, callbacks{c}
				, nativeSocket{*processorProxyPtr->getDispatcher()}
				, socketStreamCallback{[&](auto* buffer, auto size) mutable
				{
					return send(buffer, size);
				}}
				, socketStream{&socketStreamCallback}
				, opened{false}
				{
					LOG(DEBUG) << LABELS{"conn"} << "Connection object was created (id=" << this << ")";
				}

				~Connection()
				{
					stop();

					LOG(DEBUG) << LABELS{"conn"} << "Connection object was deleted (id=" << this << ")";
				}

				Connection(const Connection&) = delete;             // non-copyable
				Connection& operator=(const Connection&) = delete;  // non-assignable
				Connection(Connection&& rhs) = delete;              // non-movable
				Connection& operator=(Connection&& rhs) = delete;   // non-movable-assignable

				inline auto& getNativeSocket()
				{
					return nativeSocket;
				}

				inline auto& getSocketStream()
				{
					return socketStream;
				}

				inline auto isOpen()
				{
					return opened;
				}

				inline auto send(const void* buffer, const std::size_t size)
				{
					std::size_t sent{0};

					if (nativeSocket.is_open())
					{
						sent = nativeSocket.send(asio::buffer(buffer, size));
					}
					else
					{
						LOG(WARNING) << LABELS{"conn"} << "Could not send any data to as socket is not opened (id=" << this << ")";
					}

					return sent;
				}

				void start(asio::ip::tcp::acceptor& acceptor)
				{
					onStart();

					// TODO: validate result???
					acceptor.async_accept(
						nativeSocket,
						[&](const std::error_code& error)
						{
							processorProxyPtr->wrap([=]
							{
								onOpen(error);
							})();
						}
					);
				}

				void stop()
				{
					// it will trigger chain of callbacks
					if (nativeSocket.is_open()) try
					{
						nativeSocket.shutdown(asio::socket_base::shutdown_both);
					}
					catch(...) {}
					try
					{
						nativeSocket.close();
					}
					catch(...) {}
				}

			protected:
				void onClose(const std::error_code error)
				{
					// invoking close callback before connection is desposed and setting opened status
					if (opened)
					{
						callbacks.getCloseCallback()(*this);
					}
					opened = false;

					// stopping this connection after it's been closed
					onStop();

					LOG(DEBUG) << LABELS{"conn"} << "Connection was closed (id=" << this << ", error='" << error.message() << "')";
				}

				void onData(const std::error_code error, const std::size_t receivedSize)
				{
					// if error then closing this connection
					if (error)
					{
						onClose(error);
					}
					else
					{
						// calling onData callback that does all the usefull work
						if (receivedSize > 0)
						{
							callbacks.getDataCallback()(*this, buffer, receivedSize);
						}

						// submitting a new task here allows other tasks to progress
						processorProxyPtr->process([&]
						{
							// keep receiving data 'recursivelly' (task processor is used instead of stack)
							nativeSocket.async_read_some(
								asio::buffer(buffer, bufferSize),
								[&](const std::error_code error, std::size_t bytes_transferred)
								{
									processorProxyPtr->wrap([=]
									{
										onData(error, bytes_transferred);
									})();
								}
							);
						});
					}
				}

				void onOpen(const std::error_code error)
				{
					if (error || !nativeSocket.is_open())
					{
						onClose(error);
					}
					else
					{
						// invoking open callback and setting opened status
						if (!opened)
						{
							opened = true;
							callbacks.getOpenCallback()(*this);
						}

						// start receiving data
						onData(error, 0);

						LOG(DEBUG) << LABELS{"conn"} << "Connection was opened (id=" << this << ")";
					}
				}

				void onStart()
				{
					// invoking open callback before any connectivy action
					callbacks.getStartCallback()(*this);

					LOG(DEBUG) << LABELS{"conn"} << "Connection was started (id=" << this << ")";
				}

				void onStop()
				{
					// connection cannot be removed at this moment as this method is called withing this connection
					processorProxyPtr->process([&]
					{
						// invoking stop callback
						callbacks.getStopCallback()(*this);
					});

					LOG(DEBUG) << LABELS{"conn"} << "Connection was stopped (id=" << this << ")";
				}

			private:
				conwrap::ProcessorAsioProxy<ContainerType>* processorProxyPtr;
				CallbacksBase<Connection<ContainerType>>&   callbacks;
				asio::ip::tcp::socket                       nativeSocket;
				SocketStreamCallback                        socketStreamCallback;
				std::ostream                                socketStream;
				bool                                        opened;

				// TODO: think of some smarter way of managing buffer
				const std::size_t                           bufferSize = 1024;
				unsigned char                               buffer[1024];
		};
	}
}
