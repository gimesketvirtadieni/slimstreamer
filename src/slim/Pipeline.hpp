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

#include <chrono>
#include <conwrap/ProcessorProxy.hpp>

#include "slim/Chunk.hpp"
#include "slim/Consumer.hpp"
#include "slim/Producer.hpp"


namespace slim
{
	class Pipeline
	{
		using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

		public:
			Pipeline(Producer* p, Consumer c)
			: producerPtr{p}
			, consumer{std::move(c)} {}

			// using Rule Of Zero
		   ~Pipeline() = default;
			Pipeline(const Pipeline&) = delete;             // non-copyable
			Pipeline& operator=(const Pipeline&) = delete;  // non-assignable
			Pipeline(Pipeline&& rhs) = default;
			Pipeline& operator=(Pipeline&& rhs) = default;

			inline auto isAvailable()
			{
				bool result;

				if (pauseUntil.has_value() && pauseUntil.value() > std::chrono::steady_clock::now())
				{
					result = false;
				}
				else
				{
					result = producerPtr->isAvailable();
					pauseUntil.reset();
				}

				return result;
			}

			inline auto isProducing()
			{
				return producerPtr->isProducing();
			}

			inline void pause(unsigned int millisec)
			{
				pauseUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds{millisec};
			}

			inline bool processQuantum()
			{
				auto processed{true};

				// TODO: calculate total chunks per processing quantum
				// processing chunks as long as destination is not deferring them AND max chunks per task is not reached AND there are chunks available
				for (unsigned int count{5}; processed && count > 0 && isAvailable(); count--)
				{
					processed = producerPtr->produce(consumer);
				}

				// returning TRUE if pipeline deferes processing
				return !processed;
			}

			inline void start()
			{
				producerPtr->start([]
				{
					LOG(ERROR) << LABELS{"slim"} << "Buffer overflow error: a chunk was skipped";
				});
			}

			inline void stop(bool gracefully = true)
			{
				producerPtr->stop(gracefully);
			}

		private:
			Producer*                producerPtr;
			Consumer                 consumer;
			std::optional<TimePoint> pauseUntil{std::nullopt};
	};
}
