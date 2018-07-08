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
#include <functional>
#include <memory>
#include <thread>

#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	template <class ProducerType, class ConsumerType>
	class Scheduler
	{
		public:
			Scheduler(std::unique_ptr<ProducerType> pr, std::unique_ptr<ConsumerType> cn)
			: producerPtr{std::move(pr)}
			, consumerPtr{std::move(cn)}
			{
				LOG(DEBUG) << LABELS{"slim"} << "Scheduler object was created (id=" << this << ")";
			}

			// using Rule Of Zero
		   ~Scheduler()
			{
				LOG(DEBUG) << LABELS{"slim"} << "Scheduler object was deleted (id=" << this << ")";
			}

			Scheduler(const Scheduler&) = delete;             // non-copyable
			Scheduler& operator=(const Scheduler&) = delete;  // non-assignable
			Scheduler(Scheduler&& rhs) = delete;              // non-movable
			Scheduler& operator=(Scheduler&& rhs) = delete;   // non-move-assignable

			void setProcessorProxy(conwrap::ProcessorProxy<ContainerBase>* p)
			{
				processorProxyPtr = p;

				producerPtr->setProcessorProxy(p);
				consumerPtr->setProcessorProxy(p);
			}

			void start()
			{
				producerPtr->start();
				consumerPtr->start();

				// starting a single consumer thread for processing PCM data
				processingThread = std::thread([&]
				{
					LOG(DEBUG) << LABELS{"slim"} << "Processing thread was started (id=" << std::this_thread::get_id() << ")";

					// signalling back to the starting thread when processing thread is fully ready
					processingStarted = true;

					while (producerPtr->isRunning())
					{
						if (producerPtr->isAvailable())
						{
							submitProcessingQuantum();
						}
						else
						{
							// TODO: cruise control should be implemented
							// if no PCM data is available in any of the producers then pause processing
							std::this_thread::sleep_for(std::chrono::milliseconds{10});
						}
					}

					LOG(DEBUG) << LABELS{"slim"} << "Streamer thread was stopped (id=" << std::this_thread::get_id() << ")";
				});

				// making sure it is up and running
				while(processingThread.joinable() && !processingStarted)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds{1});
				}
			}

			void stop(bool gracefully = true)
			{
				producerPtr->stop();
				consumerPtr->stop();

				// waiting for consumer's thread to terminate
				if (processingThread.joinable())
				{
					processingThread.join();
				}
			}

		protected:
			void submitProcessingQuantum()
			{
				processorProxyPtr->process([&producer = *producerPtr, &consumer = *consumerPtr]
				{
					// TODO: calculate total chunks per processing quantum
					// processing chunks as long as consumer is not deferring them AND max chunks per task is not reached AND there are chunks available
					auto processed{true};
					for (unsigned int count{0}; processed && count < 5 && producer.isAvailable(); count++)
					{
						processed = producer.produce(consumer);
					}

					// if processing was defered then pausing it for some period
					if (!processed)
					{
						// TODO: cruise control should be implemented
						producer.pause(50);
					}
				});
			}

		private:
			std::unique_ptr<ProducerType>           producerPtr;
			std::unique_ptr<ConsumerType>           consumerPtr;
			conwrap::ProcessorProxy<ContainerBase>* processorProxyPtr{nullptr};
			std::thread                             processingThread;
			volatile bool                           processingStarted{false};
	};
}
