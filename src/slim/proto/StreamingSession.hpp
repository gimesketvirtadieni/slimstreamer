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
#include <cstring>  // std::memcpy
#include <functional>
#include <queue>
#include <memory>
#include <scope_guard.hpp>
#include <sstream>  // std::stringstream
#include <string>
#include <type_safe/optional.hpp>

#include "slim/Chunk.hpp"
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

		template<typename ConnectionType, typename StreamerType>
		class StreamingSession
		{
			public:
				StreamingSession(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pp, std::reference_wrapper<ConnectionType> co, std::reference_wrapper<StreamerType> st, std::string id, EncoderBuilder eb)
				: processorProxy{pp}
				, connection{co}
				, streamer{st}
				, clientID{id}
				{
					LOG(DEBUG) << LABELS{"proto"} << "HTTP session object was created (id=" << this << ")";

					eb.setEncodedCallback([&](const auto* encodedData, auto encodedDataSize)
					{
						for (std::size_t offset = 0, chunkSize; offset < encodedDataSize; offset += chunkSize)
						{
							auto pooledBuffer = bufferPool.allocate();
							if (!pooledBuffer.getData())
							{
								LOG(WARNING) << LABELS{"proto"} << "Transfer buffer is full - skipping encoded chunk";
								return;
							}

							// copying encoded PCM content to the allocated buffer and submitting for transfer to a client
							chunkSize = std::min(pooledBuffer.getSize(), encodedDataSize - offset);
							std::memcpy(pooledBuffer.getData(), encodedData, chunkSize);
							auto transferDataChunk = TransferDataChunk{std::move(pooledBuffer), chunkSize, 0};

							// submitting a chunk to be transferred to a client
							transferBufferQueue.push(std::move(transferDataChunk));
							processorProxy.process([&]
							{
								transferTask();
							});
						}
					});
					encoderPtr = std::move(eb.build());
				}

				~StreamingSession()
				{
					// canceling deferred operation
					ts::with(timer, [&](auto& timer)
					{
						timer.cancel();
					});

					LOG(DEBUG) << LABELS{"proto"} << "HTTP session object was deleted (id=" << this << ")";
				}

				StreamingSession(const StreamingSession&) = delete;             // non-copyable
				StreamingSession& operator=(const StreamingSession&) = delete;  // non-assignable
				StreamingSession(StreamingSession&& rhs) = delete;              // non-movable
				StreamingSession& operator=(StreamingSession&& rhs) = delete;   // non-movable-assignable

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

				inline void start()
				{
					encoderPtr->start();
					running = true;

					// creating response string
					std::stringstream ss;
					ss << "HTTP/1.1 200 OK\r\n"
					   << "Server: SlimStreamer ("
					   // TODO: provide version to the constructor
					   << VERSION
					   << ")\r\n"
					   << "Connection: close\r\n"
					   << "Content-Type: " << encoderPtr->getMIME() << "\r\n"
					   << "\r\n";

					// sending response string
					connection.get().write(ss.str());
				}

				template <typename CallbackType>
				inline void stop(CallbackType callback)
				{
					if (running)
					{
						encoderPtr->stop([&, callback = std::move(callback)]
						{
							flush([&, callback = std::move(callback)]
							{
								running = false;

								// stopping connection will submit a onClose handler
								connection.get().stop();

								// submiting a new handler is required to run a callback after onClose handler is processed
								processorProxy.process([callback = std::move(callback)]
								{
									callback();
								});
							});
						});
					}
					else
					{
						callback();
					}
				}

			protected:
				using PooledBufferType = util::buffer::BufferPool<std::uint8_t>::PooledBufferType;
				struct TransferDataChunk
				{
					PooledBufferType           buffer;
					PooledBufferType::SizeType size;
					PooledBufferType::SizeType offset;
				};

				template <typename CallbackType>
				void flush(CallbackType callback)
				{
					if (!transferring)
					{
						while (!transferBufferQueue.empty())
						{
							transferBufferQueue.pop();
						}
						callback();
					}
					else
					{
						// waiting until data is transferred
						timer = ts::ref(processorProxy.processWithDelay([&, callback = std::move(callback)]
						{
							flush(std::move(callback));
						}, std::chrono::milliseconds{1}));
					}
				}

				inline void transferTask()
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

					// it will disabling submiting write async requests from any other task until this write succeeds
					transferring = true;

					auto& transferDataChunk = transferBufferQueue.front();
					connection.get().writeAsync(transferDataChunk.buffer.getData() + transferDataChunk.offset, transferDataChunk.size, [this](auto error, auto sizeTransferred)
					{
						// using RAII guard to ensure that a buffer is removed from the queue unless it is explicitly instructed not to do so
						auto releaseBuffer = true;
						::util::scope_guard onExit = [&]
						{
							if (releaseBuffer && transferBufferQueue.size())
							{
								transferBufferQueue.pop();
							}

							// reseting transferring flag and submitting a new transfer task so it can write async
							transferring = false;
							processorProxy.process([&]
							{
								transferTask();
							});
						};

						// if case of transfer error just log the error; chunk will be removed from the queue by the RAII guard
						if (error)
						{
							LOG(ERROR) << LABELS{"proto"} << "Error while transferring data chunk: " << error.message();
							return;
						}

						// guarding against cases when queue has been flushed
						if (!transferBufferQueue.size())
						{
							return;
						}

						auto& transferDataChunk = transferBufferQueue.front();
						if (sizeTransferred < transferDataChunk.size)
						{
							LOG(INFO) << LABELS{"proto"} << "Incomplete buffer content was sent, transmitting reminder: chunk size=" << transferDataChunk.size << ", transferred=" << sizeTransferred;

							// setting up transfer chunk data to transfer only the reminder
							transferDataChunk.size   -= sizeTransferred;
							transferDataChunk.offset += sizeTransferred;

							// it will instruct the guard not to release transfer data chunk back to the pool
							releaseBuffer = false;
						}

						//LOG(DEBUG) << LABELS{"proto"} << "Chunk was successfully transferred: " << transferDataChunk.size << ", transferred=" << sizeTransferred;
					});
				}

			private:
				conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy;
				std::reference_wrapper<ConnectionType>                   connection;
				std::reference_wrapper<StreamerType>                     streamer;
				std::string                                              clientID;
				std::unique_ptr<EncoderBase>                             encoderPtr;
				bool                                                     running{false};
				bool                                                     transferring{false};
				// TODO: parameterize
				util::buffer::BufferPool<std::uint8_t>                   bufferPool{64, 4096};
				std::queue<TransferDataChunk>                            transferBufferQueue;
				util::BigInteger                                         framesProvided{0};
				ts::optional_ref<conwrap2::Timer>                        timer{ts::nullopt};
		};
	}
}
