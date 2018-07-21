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
#include <conwrap/ProcessorProxy.hpp>
#include <memory>
#include <vector>

#include "slim/log/log.hpp"


namespace slim
{
	template <class ConsumerType>
	class Demultiplexor : public Consumer
	{
		public:
			Demultiplexor(std::vector<std::unique_ptr<ConsumerType>> c)
			: Consumer{0}
			, consumers{std::move(c)}
			{
				// setting the current consumer to the first one among provided
				if (consumers.size() > 0)
				{
					currentConsumerPtr = consumers.at(0).get();
				}
			}

			// using Rule Of Zero
			virtual ~Demultiplexor() = default;
			Demultiplexor(const Demultiplexor&) = delete;             // non-copyable
			Demultiplexor& operator=(const Demultiplexor&) = delete;  // non-assignable
			Demultiplexor(Demultiplexor&& rhs) = delete;              // non-movable
			Demultiplexor& operator=(Demultiplexor&& rhs) = delete;   // non-move-assignable

			virtual bool consumeChunk(Chunk& chunk) override
			{
				auto result{false};

				if (currentConsumerPtr)
				{
					auto chunkSamplingRate{chunk.getSamplingRate()};

					// switching over to a different consumer with the same as chunk's sampling rate if needed
					if (chunkSamplingRate != currentConsumerPtr->getSamplingRate())
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

					// consuming chunk if sampling rates match
					if (chunkSamplingRate == currentConsumerPtr->getSamplingRate())
					{
						result = currentConsumerPtr->consumeChunk(chunk);
					}
					else
					{
						// consume chunk without processing if there is no matching consumer
						result = true;

						// skipping end-of-stream chunks silencely
						if (chunkSamplingRate)
						{
							LOG(WARNING) << LABELS{"slim"} << "Chunk was skipped as there is no matching consumer defined";
						}
					}
				}
				else
				{
					// consume chunk without processing if there is no any consumer defined
					result = true;
					LOG(WARNING) << LABELS{"slim"} << "Chunk was skipped as there are no consumers defined";
				}

				return result;
			}

			virtual unsigned int getSamplingRate() override
			{
				// TODO: work in progress
				return 0;
			}

			virtual void setSamplingRate(unsigned int s) override
			{
				// TODO: work in progress
			}

			virtual void start() override
			{
				std::for_each(consumers.begin(), consumers.end(), [](auto& consumerPtr)
				{
					consumerPtr->start();
				});
			}

			virtual void stop(bool gracefully = true) override
			{
				std::for_each(consumers.begin(), consumers.end(), [](auto& consumerPtr)
				{
					consumerPtr->stop();
				});
			}

		private:
			std::vector<std::unique_ptr<ConsumerType>> consumers;
			ConsumerType*                              currentConsumerPtr{nullptr};
	};
}
