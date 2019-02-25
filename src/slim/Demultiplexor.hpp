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

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	template <class ConsumerType>
	class Demultiplexor : public Consumer
	{
		public:
			Demultiplexor(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> p, std::vector<std::unique_ptr<ConsumerType>> c)
			: Consumer{p}
			, consumers{std::move(c)} {}

			// using Rule Of Zero
			virtual ~Demultiplexor() = default;
			Demultiplexor(const Demultiplexor&) = delete;             // non-copyable
			Demultiplexor& operator=(const Demultiplexor&) = delete;  // non-assignable
			Demultiplexor(Demultiplexor&& rhs) = delete;              // non-movable
			Demultiplexor& operator=(Demultiplexor&& rhs) = delete;   // non-move-assignable

			virtual bool consumeChunk(Chunk& chunk) override
			{
				auto result{false};
				auto chunkSamplingRate{chunk.getSamplingRate()};

				// if sampling rate does not match to the provided chunk's sampling rate
				if (currentConsumerPtr && currentConsumerPtr->getSamplingRate() != chunkSamplingRate)
				{
					currentConsumerPtr = nullptr;
				}

				// switching over to a different consumer with the same as chunk's sampling rate if needed
				if (!currentConsumerPtr)
				{
					for (auto& consumerPtr : consumers)
					{
						if (chunkSamplingRate == consumerPtr->getSamplingRate())
						{
							currentConsumerPtr = consumerPtr.get();
							break;
						}
					}
				}

				// if there is a consumer with the same sampling rate
				if (currentConsumerPtr)
				{
					result = currentConsumerPtr->consumeChunk(chunk);
				}
				else
				{
					// consuming chunk without processing if there is no matching consumer
					result = true;

					// skipping end-of-stream chunks silencely
					if (chunkSamplingRate)
					{
						LOG(WARNING) << LABELS{"slim"} << "Chunk was skipped as there is no matching consumer defined";
					}
				}

				return result;
			}

			virtual bool isRunning() override
			{
				auto result{false};

				if (currentConsumerPtr)
				{
					result = currentConsumerPtr->isRunning();
				}

				return result;
			}

			virtual void start() override
			{
				std::for_each(consumers.begin(), consumers.end(), [](auto& consumerPtr)
				{
					consumerPtr->start();
				});
			}

			virtual void stop(std::function<void()> callback) override
			{
				std::for_each(consumers.begin(), consumers.end(), [](auto& consumerPtr)
				{
					consumerPtr->stop([] {});
				});
			}

		private:
			std::vector<std::unique_ptr<ConsumerType>> consumers;
			ConsumerType*                              currentConsumerPtr{nullptr};
	};
}
