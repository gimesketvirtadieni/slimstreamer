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
#include <conwrap2/Timer.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <scope_guard.hpp>
#include <type_safe/optional_ref.hpp>

#include "slim/Consumer.hpp"
#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"
#include "slim/Producer.hpp"


namespace slim
{
	namespace ts = type_safe;

	class Scheduler
	{
		public:
			Scheduler(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pp, std::unique_ptr<Producer> pr, std::unique_ptr<Consumer> cn)
			: processorProxy{pp}
			, producerPtr{std::move(pr)}
			, consumerPtr{std::move(cn)}
			{
				LOG(DEBUG) << LABELS{"slim"} << "Scheduler object was created (id=" << this << ")";
			}

			// using Rule Of Zero
			~Scheduler()
			{
				// canceling deferred operation if any
				ts::with(taskTimer, [&](auto& timer)
				{
					timer.cancel();
				});

				LOG(DEBUG) << LABELS{"slim"} << "Scheduler object was deleted (id=" << this << ")";
			}

			Scheduler(const Scheduler&) = delete;             // non-copyable
			Scheduler& operator=(const Scheduler&) = delete;  // non-assignable
			Scheduler(Scheduler&& rhs) = delete;              // non-movable
			Scheduler& operator=(Scheduler&& rhs) = delete;   // non-move-assignable

			void start()
			{
				producerPtr->start();
				consumerPtr->start();

				processorProxy.process([&]
				{
					processTask();
				});

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
				taskTimer.reset();

				// defining consumer function
				std::function<bool(Chunk&)> consumer{[&](auto& chunk)
				{
					return consumerPtr->consumeChunk(chunk);
				}};

				// TODO: calculate total chunks per processing quantum
				// processing up to max(count) chunks within one event-loop quantum
				auto available{ts::optional<unsigned int>{ts::nullopt}};
				unsigned int delay{0};
				for (int count{5}; count > 0; count--) try
				{
					::util::scope_guard_failure onError = [&]
					{
						// skipping one chunk in case of an exception while producing / consuming
						available = producerPtr->skipChunk();

						LOG(WARNING) << "A chunk was skipped due to an error";
					};

					// producing / consuming chunk
					ts::with((available = producerPtr->produceChunk(consumer)), [&](auto& p)
					{
						if (p > 0)
						{
							delay = p;
							count = 0;
						}
					});
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

				// TODO: refactor
				// if there is more PCM data to be processed
				if (producerPtr->isRunning())
				{
					if (!delay)
					{
						processorProxy.process([&]
						{
							processTask();
						});
					}
					else
					{
						taskTimer = ts::ref(processorProxy.processWithDelay([&]
						{
							processTask();
						}, std::chrono::milliseconds{delay}));
					}
				}
			}

		private:
			conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy;
			std::unique_ptr<Producer>                                producerPtr;
			std::unique_ptr<Consumer>                                consumerPtr;
			ts::optional_ref<conwrap2::Timer>                        taskTimer{ts::nullopt};
	};
}
