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
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <type_safe/optional_ref.hpp>
#include <vector>

#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	namespace ts = type_safe;

	template <class ProducerType>
	class Multiplexor : public Producer
	{
		public:
			Multiplexor(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pp, std::vector<std::unique_ptr<ProducerType>> pr)
			: Producer{pp}
			, producers{std::move(pr)} {}

			// using Rule Of Zero
			virtual ~Multiplexor() = default;
			Multiplexor(const Multiplexor&) = delete;             // non-copyable
			Multiplexor& operator=(const Multiplexor&) = delete;  // non-assignable
			Multiplexor(Multiplexor&& rhs) = delete;              // non-movable
			Multiplexor& operator=(Multiplexor&& rhs) = delete;   // non-move-assignable

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

			virtual ts::optional<unsigned int> produceChunk(std::function<bool(Chunk&)>& consumer) override
			{
				auto result{ts::optional<unsigned int>{ts::nullopt}};

				// setting up a producer if needed
				if (!currentProducer.has_value())
				{
					switchToNextProducer();
				}

				ts::with(currentProducer, [&](auto& producer)
				{
					result = producer.produceChunk(consumer);
				});

				// switching to the next producer if there were no chunks produced
				if (!result.has_value())
				{
					switchToNextProducer();
				}

				return result;
			}

			virtual ts::optional<unsigned int> skipChunk() override
			{
				auto result{ts::optional<unsigned int>{ts::nullopt}};

				ts::with(currentProducer, [&](auto& producer)
				{
					result = producer.skipChunk();
				});

				return result;
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

					// keeping producer's thread in dedicated vector
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

		protected:
			inline void switchToNextProducer()
			{
				LOG(DEBUG) << LABELS{"slim"} << "SWITCH1";

				if ((++currentProducerIndex) >= producers.size())
				{
					currentProducerIndex = 0;
				}
				
				if (producers.size() > 0)
				{
					currentProducer = ts::ref(*producers.at(currentProducerIndex));
				}
				else
				{
					currentProducer.reset();
				}
			}

		private:
			std::vector<std::unique_ptr<ProducerType>> producers;
			unsigned int                               currentProducerIndex{0};
			ts::optional_ref<ProducerType>             currentProducer{ts::nullopt};
			std::vector<std::thread>                   threads;
	};
}
