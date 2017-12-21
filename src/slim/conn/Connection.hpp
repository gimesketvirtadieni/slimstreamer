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
#include <memory>

#include "slim/conn/CallbacksBase.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	namespace conn
	{
		template <typename Container>
		class Connection
		{
			public:
				Connection(conwrap::ProcessorAsioProxy<Container>* p, CallbacksBase<Connection<Container>>& c)
				: processorProxyPtr{p}
				, callbacks{c}
				, nativeSocket{*processorProxyPtr->getDispatcher()}
				, opened{false} {}

				~Connection()
				{
					stop();
				}

				Connection(const Connection&) = delete;             // non-copyable
				Connection& operator=(const Connection&) = delete;  // non-assignable
				Connection(Connection&& rhs) = delete;              // non-movable
				Connection& operator=(Connection&& rhs) = delete;   // non-movable-assignable

				inline auto isOpen()
				{
					return opened;
				}

				void start(asio::ip::tcp::acceptor& acceptor)
				{
					// invoking open callback before any connectivy action
					if (callbacks.startCallback)
					{
						callbacks.startCallback(*this);
					}

					// TODO: validate result???
					acceptor.async_accept(
						this->nativeSocket,
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

				// TODO: temporary method
				auto& getNativeSocket()
				{
					return nativeSocket;
				}

			protected:
				void onClose(const std::error_code error)
				{
					LOG(DEBUG) << LABELS{"slim"} << "Closing connection (id=" << this << ", error='" << error.message() << "')...";

					// invoking close callback before connection is desposed and setting opened status
					if (opened && callbacks.closeCallback)
					{
						callbacks.closeCallback(*this);
					}
					opened = false;

					// stopping this connection after it's been closed
					onStop();

					LOG(DEBUG) << LABELS{"slim"} << "Connection was closed (id=" << this << ")";
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
						if (receivedSize > 0 && callbacks.dataCallback)
						{
							callbacks.dataCallback(*this, buffer, receivedSize);
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
						LOG(DEBUG) << LABELS{"slim"} << "Opening connection (id=" << this << ")...";

						// invoking open callback and setting opened status
						if (!opened)
						{
							opened = true;
							if (callbacks.openCallback)
							{
								callbacks.openCallback(*this);
							}
						}

						// start receiving data
						onData(error, 0);

						LOG(DEBUG) << LABELS{"slim"} << "Connection was opened (id=" << this << ")";
					}
				}

				void onStop()
				{
					// connection cannot be removed at this moment as this method is called withing this connection
					processorProxyPtr->process([&]
					{
						// invoking stop callback
						if (callbacks.stopCallback)
						{
							callbacks.stopCallback(*this);
						}
					});
				}

			private:
				conwrap::ProcessorAsioProxy<Container>* processorProxyPtr;
				CallbacksBase<Connection<Container>>&   callbacks;
				asio::ip::tcp::socket                   nativeSocket;
				bool                                    opened;

				// TODO: think of some smarter way of managing buffer
				const std::size_t                       bufferSize = 1024;
				unsigned char                           buffer[1024];
		};
	}
}
