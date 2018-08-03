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
#include <memory>
#include <vector>

#include "slim/Exception.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	template <class ConsumerType>
	class Demultiplexor : public Consumer
	{
		public:
			Demultiplexor(std::vector<std::unique_ptr<ConsumerType>> c)
			: Consumer{0}
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

			virtual unsigned int getSamplingRate() override
			{
				unsigned int sr{0};

				if (currentConsumerPtr)
				{
					sr = currentConsumerPtr->getSamplingRate();
				}

				return sr;
			}

			virtual void setSamplingRate(unsigned int s) override
			{
				throw Exception("Sampling rate cannot be set for demultiplexor");
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
