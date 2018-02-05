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
#include "slim/ContainerBase.hpp"


namespace slim
{
	template<typename Source, typename Destination>
	class Pipeline
	{
		using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

		public:
			Pipeline(Source s, Destination d)
			: source{std::move(s)}
			, destination{std::move(d)} {}

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
					result = source.isAvailable();
					pauseUntil.reset();
				}

				return result;
			}

			inline auto isProducing()
			{
				return source.isProducing();
			}

			inline void onProcess()
			{
				// making sure pipeline is not paused
				if (!pauseUntil.has_value() || pauseUntil.value() < std::chrono::steady_clock::now())
				{
					pauseUntil.reset();

					// TODO: calculate maxChunks per processing quantum
					unsigned int maxChunks{5};

					// no need to return defer status to the scheduler as deferring chunks is handled by a pipeline, source and destination
					auto processed{true};

					// processing chunks as long as destination is not deferring them AND max chunks per task is not reached AND there are chunks available
					for (unsigned int count{0}; processed && count < maxChunks && isAvailable(); count++)
					{
						processed = source.supply([&](Chunk& chunk)
						{
							return destination.consume(chunk);
						});
					}

					// if it was not possible to process a chunk then pausing this pipeline
					// TODO: calculate optimal pause timeout
					if (!processed)
					{
						pause(50);
					}
				}
			}

			inline void start()
			{
				source.start([]
				{
					LOG(ERROR) << LABELS{"slim"} << "Buffer overflow error: a chunk was skipped";
				});
			}

			inline void stop(bool gracefully = true)
			{
				source.stop(gracefully);
			}

		protected:
			inline void pause(unsigned int millisec)
			{
				pauseUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds{millisec};
			}

		private:
			Source                   source;
			Destination              destination;
			std::optional<TimePoint> pauseUntil{std::nullopt};
	};
}
