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

#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	namespace ts = type_safe;

	template <class ProducerType, class ConsumerType>
	class Scheduler
	{
		public:
			Scheduler(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pp, std::unique_ptr<ProducerType> pr, std::unique_ptr<ConsumerType> cn)
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

			inline bool isRunning()
			{
				return producerPtr->isRunning() || consumerPtr->isRunning();
			}

			inline void start()
			{
				producerPtr->start();
				consumerPtr->start();

				processorProxy.process([&]
				{
					processTask();
				});
			}

			inline void stop(std::function<void()> callback)
			{
				producerPtr->stop([&, callback = std::move(callback)]
				{
					consumerPtr->stop(std::move(callback));
				});
			}

		protected:
			void processTask()
			{
				auto delayProcessing{std::chrono::milliseconds{0}};

				// TODO: should it be with(taskTime){taskTime.cancel()}?
				taskTimer.reset();

				try
				{
					// this safe guard is meant for capturing consumer's errors
					::util::scope_guard_failure onError = [&]
					{
						stop([] {});
					};

					// TODO: calculate total chunks per processing quantum
					// processing up to max(chunksToProcess) chunks within one event-loop quantum
					for (int chunksToProcess{5}; chunksToProcess > 0 && !delayProcessing.count(); chunksToProcess--)
					{
						// producing / consuming chunk
						delayProcessing = producerPtr->produceChunk([&](auto& chunk)
						{
							return consumerPtr->consumeChunk(chunk);
						}).value_or(std::chrono::milliseconds{0});
					}
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
				if (isRunning())
				{
					if (delayProcessing.count() > 0)
					{
						taskTimer = ts::ref(processorProxy.processWithDelay([&]
						{
							processTask();
						}, delayProcessing));
					}
					else
					{
						processorProxy.process([&]
						{
							processTask();
						});
					}
				}
			}

		private:
			conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy;
			std::unique_ptr<ProducerType>                            producerPtr;
			std::unique_ptr<ConsumerType>                            consumerPtr;
			ts::optional_ref<conwrap2::Timer>                        taskTimer{ts::nullopt};
	};
}
