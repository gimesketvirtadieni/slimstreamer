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

#include <atomic>
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
			, monitorThread
			{[&]{
				// starting a single consumer thread for processing PCM data
				LOG(DEBUG) << LABELS{"slim"} << "Scheduler monitor thread was started (id=" << std::this_thread::get_id() << ")";

				while (!finish)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds{50});

					if (requestTaskLater)
					{
						requestTaskLater = false;

						processorProxyPtr->process([&]
						{
							processTask();
						});
					}
				}

				LOG(DEBUG) << LABELS{"slim"} << "Scheduler monitor thread was stopped (id=" << std::this_thread::get_id() << ")";
			}}
			{
				LOG(DEBUG) << LABELS{"slim"} << "Scheduler object was created (id=" << this << ")";
			}

			// using Rule Of Zero
		   ~Scheduler()
			{
				// signalling monitor thread to stop and joining it
				finish = true;
				if (monitorThread.joinable())
				{
					monitorThread.join();
				}

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

				// asking monitor thread to submit a new task
				requestTaskLater = true;

				LOG(DEBUG) << LABELS{"slim"} << "Streaming was started";
			}

			void stop(bool gracefully = true)
			{
				producerPtr->stop();
				consumerPtr->stop();

				LOG(DEBUG) << LABELS{"slim"} << "Streaming was stopped";
			}

		protected:
			void processTask()
			{
				auto available{true};

				// TODO: calculate total chunks per processing quantum
				// processing up to max(count) chunks within one event-loop quantum
				for (unsigned int count{0}; available && count < 5; available = producerPtr->produceChunk(*consumerPtr), count++) {}

				// if there is more PCM data to be processed
				if (available)
				{
					processorProxyPtr->process([&]
					{
						processTask();
					});
				}
				else if (producerPtr->isRunning())
				{
					// requesting monitoring thread to submit a new task later, which will allow event-loop to progress
					requestTaskLater = true;
				}
			}

		private:
			std::unique_ptr<ProducerType>           producerPtr;
			std::unique_ptr<ConsumerType>           consumerPtr;
			std::thread                             monitorThread;
			std::atomic<bool>                       finish{false};
			std::atomic<bool>                       requestTaskLater{false};
			conwrap::ProcessorProxy<ContainerBase>* processorProxyPtr{nullptr};
	};
}
