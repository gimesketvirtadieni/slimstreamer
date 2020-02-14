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

#include <algorithm>  // std::min
#include <conwrap2/ProcessorProxy.hpp>
#include <conwrap2/Timer.hpp>
#include <cstring>  // std::memcpy
#include <queue>
#include <memory>
#include <ofats/invocable.h>
#include <scope_guard.hpp>
#include <sstream>  // std::stringstream
#include <string>
#include <type_safe/optional.hpp>

#include "slim/Chunk.hpp"
#include "slim/ContainerBase.hpp"
#include "slim/EncoderBase.hpp"
#include "slim/EncoderBuilder.hpp"
#include "slim/log/log.hpp"
#include "slim/util/BigInteger.hpp"
#include "slim/util/buffer/BufferPool.hpp"


namespace slim
{
	namespace proto
	{
		namespace ts = type_safe;

		template<typename ConnectionType>
		class StreamingSession
		{
			public:
				StreamingSession(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pp, std::reference_wrapper<ConnectionType> co, EncoderBuilder eb)
				: processorProxy{pp}
				, connection{co}
				{
					LOG(DEBUG) << LABELS{"proto"} << "HTTP session object was created (id=" << this << ")";

					eb.setEncodedCallback([&](const auto* encodedData, auto encodedDataSize)
					{
						submitData(encodedData, encodedDataSize);
					});
					encoderPtr = std::move(eb.build());
				}

				~StreamingSession()
				{
					LOG(DEBUG) << LABELS{"proto"} << "HTTP session object was deleted (id=" << this << ")";
				}

				// StreamingSession may not be moveable as it submits handlers to the processor that capture 'this'
				StreamingSession(const StreamingSession&) = delete;              // non-copyable
				StreamingSession& operator=(const StreamingSession&) = delete;   // non-assignable
				StreamingSession(const StreamingSession&&) = delete;             // non-movable
				StreamingSession& operator=(const StreamingSession&&) = delete;  // non-move-assignable

				inline bool consumeChunk(const Chunk& chunk)
				{
					if (running)
					{
						// TODO: configure
						// if no enough place in the buffer then signalling that chunk was not consumed (streamer will redeliver this chunk)
						if (bufferPool.getAvailableSize() < 10)
						{
							return false;
						}

						// sampling rates do not match then stopping this session
						if (chunk.samplingRate != encoderPtr->getSamplingRate())
						{
							// stopping this session due to incorrect data provided
							LOG(WARNING) << LABELS{"proto"} << "Closing HTTP connection due to different sampling rate used by a client (session rate=" << encoderPtr->getSamplingRate() << "; data rate=" << chunk.samplingRate << ")";
							stop([] {});
							return true;
						}

						// if this is the last chunk for the ongoing stream then stopping this session
						if (chunk.endOfStream)
						{
							stop([] {});
						}

						encoderPtr->encode(chunk.buffer.getData(), chunk.frames * chunk.bytesPerSample * chunk.channels);
						framesProvided += chunk.frames;
					}

					return true;
				}

				inline auto getClientID()
				{
					return clientID;
				}

				inline auto getConnection()
				{
					return connection;
				}

				inline auto getFramesProvided()
				{
					return framesProvided;
				}

				inline bool isRunning()
				{
					return running;
				}

				inline void onRequest(unsigned char* data, std::size_t size)
				{
					if (running)
					{
						// TODO: make more strick validation
						std::string get{"GET"};
						std::string s{(char*)data, get.size()};
						if (get.compare(s))
						{
							// stopping this session due an error
							LOG(WARNING) << LABELS{"proto"} << "Wrong HTTP method provided";
							stop([] {});
						}
					}
				}

				static auto parseClientID(std::string header)
				{
					auto result{ts::optional<std::string>{ts::nullopt}};
					auto separator{std::string{"="}};
					auto index{header.find(separator)};

					if (index != std::string::npos)
					{
						result = std::string{header.c_str() + index + separator.length(), header.length() - index - separator.length()};
					}

					return result;
				}

				inline void setClientID(ts::optional<std::string> c)
				{
					clientID = c;
				}

				inline void start()
				{
					encoderPtr->start();
					running = true;

					// creating response string
					std::stringstream ss;
					ss << "HTTP/1.1 200 OK\r\n"
					   // TODO: provide version to the constructor
					   << "Server: SlimStreamer (" << VERSION << ")\r\n"
					   << "Connection: close\r\n"
					   << "Content-Type: " << encoderPtr->getMIME() << "\r\n"
					   << "\r\n";

					// sending response string
					connection.get().write(ss.str());
				}

				template <typename CallbackType>
				inline void stop(CallbackType callback)
				{
					// guarding against non-running state
					if (!running)
					{
						callback();
						return;
					}

					// changing state to 'not running' early enough, so that no more PCM data is accepted
					running = false;

					// once encoder is stopped, flushing transfer buffer
					encoderPtr->stop([&, callback = std::move(callback)]() mutable
					{
						flush([&, callback = std::move(callback)]() mutable
						{
							// stopping connection will submit a onClose handler
							connection.get().stop();

							// submiting a new handler is required to run a callback after onClose handler is processed
							processorProxy.process([&, callback = std::move(callback)]() mutable
							{
								callback();
							});
						});
					});
				}

				inline void submitData(const unsigned char* data, const std::size_t& size)
				{
					for (std::size_t offset = 0, chunkSize; offset < size; offset += chunkSize)
					{
						auto pooledBuffer = bufferPool.allocate();
						if (!pooledBuffer.getData())
						{
							LOG(WARNING) << LABELS{"proto"} << "Transfer buffer is full - skipping encoded chunk";
							return;
						}

						// copying encoded PCM content to the allocated buffer and submitting for transfer to a client
						chunkSize = std::min(pooledBuffer.getSize(), size - offset);
						std::memcpy(pooledBuffer.getData(), data + offset, chunkSize);

						// submitting a chunk to be transferred to a client
						submitData(TransferDataChunk{std::move(pooledBuffer), chunkSize, 0, std::move([] {})});
					}
				}

			protected:
				using PooledBufferType = util::buffer::BufferPool<std::uint8_t>::PooledBufferType;

				struct TransferDataChunk
				{
					PooledBufferType             buffer;
					PooledBufferType::SizeType   size;
					PooledBufferType::SizeType   offset;
					ofats::any_invocable<void()> callback;
				};

				template <typename CallbackType>
				void flush(CallbackType callback)
				{
					// submitting an 'empty' chunk with provided callback which will be notified when transfer queue is empty
					submitData(TransferDataChunk{PooledBufferType{0, 0}, 0, 0, std::move([&, callback = std::move(callback)]() mutable
					{
						callback();
					})});
				}

				inline void submitData(TransferDataChunk&& transferDataChunk)
				{
					transferBufferQueue.push(std::move(transferDataChunk));
					transferData();
				}

				inline void transferData()
				{
					// there is nothing to transfer
					if (!transferBufferQueue.size())
					{
						return;
					}

					// there is already an ongoing transfer so no need to do anything
					if (transferring)
					{
						return;
					}

					// it will disable submiting new requests to writeAsync(...) until the current write succeeds
					transferring = true;

					auto& transferDataChunk = transferBufferQueue.front();
					connection.get().writeAsync(transferDataChunk.buffer.getData() + transferDataChunk.offset, transferDataChunk.size - transferDataChunk.offset, [this](auto error, auto sizeTransferred)
					{
						// using RAII guard to ensure that a buffer is removed from the queue unless it is explicitly instructed not to do so
						auto releaseBuffer = true;
						::util::scope_guard onExit = [&]
						{
							transferring = false;

							// releasing buffer unless guard was 'disarmed'
							if (releaseBuffer && transferBufferQueue.size())
							{
								transferBufferQueue.front().callback();
								transferBufferQueue.pop();
							}

							// if there is more data to be transferred then call transfer task
							if (transferBufferQueue.size())
							{
								transferData();
							}
						};

						// in case of transfer error just log the error, chunk will be removed from the queue by the RAII guard
						if (error)
						{
							LOG(ERROR) << LABELS{"proto"} << "Error while transferring data chunk: " << error.message();
							return;
						}

						auto& transferDataChunk = transferBufferQueue.front();
						if (sizeTransferred < transferDataChunk.size - transferDataChunk.offset)
						{
							LOG(INFO) << LABELS{"proto"} << "Incomplete buffer content was sent, transmitting remainder: chunk size=" << transferDataChunk.size << ", transferred=" << sizeTransferred;

							// setting up transfer chunk data to transfer only the reminder
							transferDataChunk.offset += sizeTransferred;

							// it will instruct the guard not to release transfer data chunk back to the pool
							releaseBuffer = false;
						}
					});
				}

			private:
				conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy;
				std::reference_wrapper<ConnectionType>                   connection;
				std::unique_ptr<EncoderBase>                             encoderPtr;
				ts::optional<std::string>                                clientID;
				bool                                                     running{false};
				bool                                                     transferring{false};
				// TODO: parameterize
				util::buffer::BufferPool<std::uint8_t>                   bufferPool{64, 4096};
				std::queue<TransferDataChunk>                            transferBufferQueue;
				util::BigInteger                                         framesProvided{0};
		};
	}
}
