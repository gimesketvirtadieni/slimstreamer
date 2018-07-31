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
#include <scope_guard.hpp>
#include <thread>

#include "slim/Consumer.hpp"
#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"
#include "slim/Producer.hpp"


namespace slim
{
	class Scheduler
	{
		public:
			Scheduler(std::unique_ptr<Producer> pr, std::unique_ptr<Consumer> cn)
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

				// starting scheduler monitor thread
				monitorThread = std::move(std::thread{[&]
				{
					LOG(DEBUG) << LABELS{"slim"} << "Scheduler monitor thread was started (id=" << std::this_thread::get_id() << ")";

					for (;!monitorFinish; std::this_thread::sleep_for(std::chrono::milliseconds{100}))
					{
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
				}});

				// asking monitor thread to submit a new task
				requestTaskLater = true;

				LOG(DEBUG) << LABELS{"slim"} << "Streaming was started";
			}

			void stop(bool gracefully = true)
			{
				producerPtr->stop();
				consumerPtr->stop();

				// signalling monitor thread to stop and joining it
				monitorFinish = true;
				if (monitorThread.joinable())
				{
					monitorThread.join();
				}

				LOG(DEBUG) << LABELS{"slim"} << "Streaming was stopped";
			}

		protected:
			void processTask()
			{
				// TODO: calculate total chunks per processing quantum
				// processing up to max(count) chunks within one event-loop quantum
				auto available{true};
				for (unsigned int count{0}; available && count < 5; count++) try
				{
					::util::scope_guard_failure onError = [&]
					{
						// skipping one chunk in case of an exception while producing / consuming
						available = producerPtr->skipChunk();

						LOG(WARNING) << "A chunk was skipped due to an error";
					};

					available = producerPtr->produceChunk(*consumerPtr);
				}
				catch (const Exception& error)
				{
					LOG(ERROR) << error;
				}
				catch (const std::exception& error)
				{
					LOG(ERROR) << error.what();
				}
				catch (...)
				{
					std::cout << "Unexpected exception" << std::endl;
				}

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
			std::unique_ptr<Producer>               producerPtr;
			std::unique_ptr<Consumer>               consumerPtr;
			std::thread                             monitorThread;
			std::atomic<bool>                       monitorFinish{false};
			std::atomic<bool>                       requestTaskLater{false};
			conwrap::ProcessorProxy<ContainerBase>* processorProxyPtr{nullptr};
	};
}
