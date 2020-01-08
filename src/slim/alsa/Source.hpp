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

#include <alsa/asoundlib.h>
#include <atomic>
#include <chrono>
#include <conwrap2/Processor.hpp>
#include <conwrap2/ProcessorProxy.hpp>
#include <cstddef>   // std::size_t
#include <cstdint>   // std::u..._t types
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <type_safe/optional.hpp>

#include "slim/alsa/Parameters.hpp"
#include "slim/Chunk.hpp"
#include "slim/Consumer.hpp"
#include "slim/ContainerBase.hpp"
#include "slim/Exception.hpp"
#include "slim/log/log.hpp"
#include "slim/util/RealTimeQueue.hpp"
#include "slim/util/BigInteger.hpp"


namespace slim
{
	namespace alsa
	{
		namespace ts = type_safe;

		enum class StreamMarker : unsigned char
		{
			beginningOfStream = 1,
			endOfStream       = 2,
			data              = 3,
		};

		class Source
		{
			using QueueType = util::RealTimeQueue<Chunk>;

			public:
				Source(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pp, Parameters pa, std::function<void()> oc = [] {})
				: parameters{pa}
				, overflowCallback{std::move(oc)}
				, queue{parameters.getQueueSize(), std::move([&](Chunk& chunk)
				{
					// no need to store data from the last channel as it contains commands
					chunk.allocateBuffer(pa.getFramesPerChunk() * pa.getLogicalChannels() * (pa.getBitsPerSample() >> 3));
				})} {}

				~Source()
				{
					// it is safe to call stop method multiple times
					stop([] {});
				}

				Source(const Source&) = delete;             // non-copyable
				Source& operator=(const Source&) = delete;  // non-assignable
				Source(Source&& rhs) = delete;              // non-movable
				Source& operator=(Source&& rhs) = delete;   // non-move-assignable

				inline auto getParameters()
				{
					return parameters;
				}

				inline bool isRunning()
				{
					return running;
				}

				template<typename ConsumerType>
				inline ts::optional<std::chrono::milliseconds> produceChunk(const ConsumerType& consumer)
				{
					auto result{ts::optional<std::chrono::milliseconds>{ts::nullopt}};

					result = producer(consumer);

					return result;
				}

				inline ts::optional<std::chrono::milliseconds> skipChunk()
				{
					// using an empty lambda that returns 'true' to 'consume' chunk without a real consumer
					return producer([](auto&)
					{
						return true;
					});
				}

				void produce();

				void start()
				{
					std::scoped_lock<std::mutex> lockGuard{threadLock};
					if (!running)
					{
						running = true;

						// starting PCM data producer thread for Real-Time processing
						producerThread = std::thread{[&]
						{
							LOG(DEBUG) << LABELS{"slim"} << "PCM data capture thread was started (id=" << std::this_thread::get_id() << ")";

							try
							{
								// opening ALSA device in a thread-safe way
								{
									std::scoped_lock<std::mutex> lockGuard{deviceLock};
									open();
								}

								// start producing
								produce();
							}
							catch (const Exception& error)
							{
								LOG(ERROR) << LABELS{"slim"} << "Error in producer thread: " << error;
							}
							catch (const std::exception& error)
							{
								LOG(ERROR) << LABELS{"slim"} << "Error in producer thread: " << error.what();
							}
							catch (...)
							{
								LOG(ERROR) << LABELS{"slim"} << "Unexpected exception";
							}

							// closing ALSA device in a thread-safe way
							{
								std::scoped_lock<std::mutex> lockGuard{deviceLock};
								close();
							};

							LOG(DEBUG) << LABELS{"slim"} << "PCM data capture thread was stopped (id=" << std::this_thread::get_id() << ")";
						}};

						// this is an optional delay for producer thread to start
						std::this_thread::sleep_for(std::chrono::milliseconds{10});
					}
				}

				template<typename CallbackType>
				inline void stop(CallbackType callback)
				{
					{
						std::scoped_lock<std::mutex> lockGuard{threadLock};
						if (running)
						{
							// issuing a request to stop receiving PCM data; it is protected by deviceLock to prevent interference with open/close procedures
							{
								std::scoped_lock<std::mutex> lockGuard{deviceLock};
								if (int result; (result = snd_pcm_drop(handlePtr)) < 0)
								{
									LOG(ERROR) << LABELS{"alsa"} << formatError("Error while stopping PCM stream unconditionally", result);
								}
							}

							// changing state to 'not running'
							running = false;
						}

						// waiting producer thread to terminate
						if (producerThread.joinable())
						{
							producerThread.join();
						}
					}

					callback();
				}

			protected:
				void              close() noexcept;
				snd_pcm_sframes_t containsData(unsigned char* buffer, snd_pcm_uframes_t frames);
				snd_pcm_uframes_t copyData(unsigned char* srcBuffer, unsigned char* dstBuffer, snd_pcm_uframes_t frames);

				inline std::string formatError(std::string message, int error = 0)
				{
					return message + ": name='" + parameters.getDeviceName() + (error != 0 ? std::string{"' error='"} + snd_strerror(error) + "'" : "");
				}

				void open();

				template<typename ConsumerType>
				inline ts::optional<std::chrono::milliseconds> producer(const ConsumerType& consumer)
				{
					auto result{ts::optional<std::chrono::milliseconds>{ts::nullopt}};

					queue.dequeue(std::move([&](Chunk& chunk) -> bool  // 'mover' function
					{
						auto consumed{false};

						consuming = true;

						// feeding consumer with a chunk
						if (consumer(chunk))
						{
							consumed = true;

							// if chunk was consumed and it is the end of the stream
							if (chunk.endOfStream)
							{
								consuming = false;
							}
							else
							{
								result = 0;
							}
						}
						else
						{
							// if consumer did not accept a chunk then deferring further processing
							// TODO: cruise control should be implemented
							result = 10;
						}

						return consumed;
					}), std::move([&]  // underflow callback
					{
						if (consuming)
						{
							result = 10;
						}
					}));

					// if there are more chunks to be consumed
					return result;
				}

				bool restore(snd_pcm_sframes_t error);

			private:
				Parameters            parameters;
				std::function<void()> overflowCallback;
				std::thread           producerThread;
				QueueType             queue;
				snd_pcm_t*            handlePtr{nullptr};
				std::atomic<bool>     running{false};
				bool                  producing{false};
				bool                  consuming{false};
				std::mutex            deviceLock;
				std::mutex            threadLock;

				util::BigInteger      chunkSequenceNumber{0};
				util::BigInteger      capturedFrames{0};
		};
	}
}
