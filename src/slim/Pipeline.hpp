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

			inline bool isAvailable()
			{
				auto result{source.isAvailable()};

				if (pauseRequestAt.has_value())
				{
					auto diff{std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - pauseRequestAt.value()).count()};

					if (diff > pauseMillisec)
					{
						pauseRequestAt = std::nullopt;
						pauseMillisec  = 0;
					}
					else
					{
						result = false;
					}
				}

				return result;
			}

			inline bool isProducing()
			{
				return source.isProducing();
			}

			inline void onProcess(unsigned int maxChunks)
			{
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
				if (!processed)
				{
					pause(100);
				}
			}

			inline void start()
			{
				source.start([]
				{
					LOG(ERROR) << "Buffer overflow: a chunk was skipped";
				});
			}

			inline void stop(bool gracefully = true)
			{
				source.stop(gracefully);
			}

		protected:
			inline void pause(unsigned int millisec)
			{
				pauseRequestAt = std::chrono::steady_clock::now();
				// TODO: use logic pauseRequestAt + millisec
				pauseMillisec = millisec;
			}

		private:
			Source      source;
			Destination destination;
			std::optional<TimePoint> pauseRequestAt{std::nullopt};
			unsigned int             pauseMillisec;
	};
}
