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

#include <conwrap/ProcessorProxy.hpp>
#include <functional>  // reference_wrapper
#include <thread>
#include <vector>

#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	template <class ProducerType>
	class Multiplexor : public Producer
	{
		public:
			Multiplexor(std::vector<std::unique_ptr<ProducerType>> p)
			: producers{std::move(p)} {}

			// using Rule Of Zero
			virtual ~Multiplexor() = default;
			Multiplexor(const Multiplexor&) = delete;             // non-copyable
			Multiplexor& operator=(const Multiplexor&) = delete;  // non-assignable
			Multiplexor(Multiplexor&& rhs) = delete;              // non-movable
			Multiplexor& operator=(Multiplexor&& rhs) = delete;   // non-move-assignable

			virtual bool isAvailable() override
			{
				auto result{false};

				if (currentProducerPtr)
				{
					result = currentProducerPtr->isAvailable();
				}

				if (!result)
				{
					for (auto& producerPtr : producers)
					{
						if (producerPtr->isAvailable())
						{
							currentProducerPtr = producerPtr.get();
							result             = true;
							break;
						}
					}
				}

				return result;
			}

			virtual bool isRunning() override
			{
				auto result{false};

				for (auto& producerPtr : producers)
				{
					if (producerPtr->isRunning())
					{
						result = true;
						break;
					}
				}

				return result;
			}

			virtual bool produceChunk(Consumer& consumer) override
			{
				auto available{false};

				if (currentProducerPtr)
				{
					available = currentProducerPtr->produceChunk(consumer);
				}

				// isAvailable will switch over to a different producer if there is no more chunks available
				if (!available)
				{
					available = isAvailable();
				}

				return available;
			}

			virtual void start() override
			{
				for (auto& producerPtr : producers)
				{
					// starting PCM data producer thread for Real-Time processing
					std::thread producerThread
					{
						[&producer = *producerPtr]
						{
							LOG(DEBUG) << LABELS{"slim"} << "PCM data capture thread was started (id=" << std::this_thread::get_id() << ")";

							try
							{
								producer.start();
							}
							catch (const Exception& error)
							{
								LOG(ERROR) << LABELS{"slim"} << "Error in producer thread: " << error;
							}
							catch (const std::exception& error)
							{
								LOG(ERROR) << LABELS{"slim"} << "Error in producer thread: " << error.what();
							}

							LOG(DEBUG) << LABELS{"slim"} << "PCM data capture thread was stopped (id=" << std::this_thread::get_id() << ")";
						}
					};

					// making sure it is up and running
					while (producerThread.joinable() && !producerPtr->isRunning())
					{
						std::this_thread::sleep_for(std::chrono::milliseconds{1});
					}

					// adding producer thread for Real-Time processing
					threads.push_back(std::move(producerThread));
				}
			}

			virtual void stop(bool gracefully = true) override
			{
				// signalling all threads to stop processing
				for (auto& producerPtr : producers)
				{
					producerPtr->stop(gracefully);
				}

				// waiting for all threads to terminate
				for (auto& thread : threads)
				{
					if (thread.joinable())
					{
						thread.join();
					}
				}
			}

		private:
			std::vector<std::unique_ptr<ProducerType>> producers;
			ProducerType*                              currentProducerPtr{nullptr};
			std::vector<std::thread>                   threads;
	};
}
