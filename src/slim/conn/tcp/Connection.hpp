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

#include <conwrap2/ProcessorProxy.hpp>
#include <cstddef>       // std::size_t
#include <exception>     // std::exception
#include <memory>
#include <system_error>  // std::system_error

#include "slim/conn/tcp/CallbacksBase.hpp"
#include "slim/log/log.hpp"
#include "slim/util/AsyncWriter.hpp"
#include "slim/util/buffer/RawBuffer.hpp"
#include "slim/util/Timestamp.hpp"


namespace slim
{
	namespace conn
	{
		namespace tcp
		{
			template <typename ContainerType>
			class Connection : public util::AsyncWriter
			{
				public:
					Connection(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> p, CallbacksBase<Connection<ContainerType>>& c)
					: processorProxy{p}
					, callbacks{c}
					, nativeSocket{processorProxy.getDispatcher()}
					, opened{false}
					{
						LOG(DEBUG) << LABELS{"conn"} << "Connection object was created (id=" << this << ")";
					}

					virtual ~Connection()
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

					inline auto isOpen()
					{
						return opened;
					}

					virtual void rewind(const std::streampos pos) override {}

					void setNoDelay(bool noDelay)
					{
						// enabling / disabling Nagle's algorithm
						nativeSocket.set_option(std::experimental::net::ip::tcp::no_delay{noDelay});
					}

					void setQuickAcknowledgment(bool quickAcknowledgment)
					{
						const std::experimental::net::detail::socket_option::boolean<IPPROTO_TCP, TCP_QUICKACK> quickack{quickAcknowledgment};
						nativeSocket.set_option(quickack);
					}

					void start(std::experimental::net::ip::tcp::acceptor& acceptor)
					{
						onStart();

						acceptor.async_accept(
							[&](auto error, auto s)
							{
								// TODO: figure out a better alternative in case when error != 0
								nativeSocket = std::move(s);

								if (!error)
								{
									// making sure synchronous operations do not throw would_block exception
									nativeSocket.non_blocking(false);

									// making sure keep-alive packets are sent
									nativeSocket.set_option(std::experimental::net::socket_base::keep_alive{true});
								}

								onOpen(error);
							}
						);
					}

					void stop()
					{
						// it will trigger chain of callbacks
						if (nativeSocket.is_open()) try
						{
							nativeSocket.shutdown(std::experimental::net::socket_base::shutdown_both);
						}
						catch(...) {}
						try
						{
							nativeSocket.close();
						}
						catch(...) {}
					}

					// including write overloads
					using AsyncWriter::write;

					virtual std::size_t write(const void* data, const std::size_t size) override
					{
						std::size_t result{0};

						if (nativeSocket.is_open()) try
						{
							result = std::experimental::net::write(nativeSocket, std::experimental::net::const_buffer(data, size));
						}
						catch(const std::system_error& e)
						{
							LOG(ERROR) << LABELS{"conn"} << "Could not send data due to an error (id=" << this << ", error=" << e.what() << ")";
						}
						else
						{
							LOG(DEBUG) << LABELS{"conn"} << "Could not send data as socket is not opened (id=" << this << ")";
						}

						return result;
					}

					// including writeAsync overloads
					using AsyncWriter::writeAsync;

					virtual void writeAsync(const void* data, const std::size_t size, util::WriteCallback callback = [](auto, auto) {}) override
					{
						if (nativeSocket.is_open())
						{
							std::experimental::net::async_write(
								nativeSocket,
								std::experimental::net::const_buffer(data, size),
								[=](const std::error_code error, const std::size_t bytes_transferred)
								{
									callback(error, bytes_transferred);
								});
						}
						else
						{
							LOG(DEBUG) << LABELS{"conn"} << "Could not send data as socket is not opened (id=" << this << ")";

							// calling callback even if socket is closed
							callback(std::error_code{std::experimental::net::error::not_connected}, 0);
						}
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

					void onData(const std::error_code error, const std::size_t receivedSize, const util::Timestamp timestamp)
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
								callbacks.getDataCallback()(*this, buffer.getBuffer(), receivedSize, timestamp);
							}

							// keep receiving data
							nativeSocket.async_read_some(
								std::experimental::net::mutable_buffer(buffer.getBuffer(), buffer.getSize()),
								[&](const std::error_code error, std::size_t bytes_transferred)
								{
									onData(error, bytes_transferred, util::Timestamp());
								}
							);
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
							onData(error, 0, util::Timestamp{});

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
						// connection cannot be removed here so submitting a handler with a callback
						processorProxy.process([&]
						{
							// invoking stop callback
							callbacks.getStopCallback()(*this);
						});

						LOG(DEBUG) << LABELS{"conn"} << "Connection was stopped (id=" << this << ")";
					}

				private:
					conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy;
					CallbacksBase<Connection<ContainerType>>&                callbacks;
					std::experimental::net::ip::tcp::socket                  nativeSocket;
					bool                                                     opened;
					// TODO: parametrize
					util::RawBuffer<std::uint8_t>                            buffer{1024};
			};
		}
	}
}
