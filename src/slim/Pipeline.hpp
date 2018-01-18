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

#include "slim/Chunk.hpp"


namespace slim
{
	template<typename Source, typename Destination>
	class Pipeline
	{
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
				return source.isAvailable();
			}

			inline bool isProducing()
			{
				return source.isProducing();
			}

			// TODO: should return exit reason (max was reached|suplier signalled with done|no more available chunks)
			inline void processChunks(unsigned int maxChunks)
			{
				// no need to return defer status to the scheduler as deferring chunks is handled by a pipeline, source and destination
				auto done{true};

				// processing chunks as long as destination is not deferring them AND max chunks per task is not reached AND there are chunks available
				for (unsigned int count{0}; done && count < maxChunks && source.isAvailable(); count++)
				{
					done = source.supply([&](Chunk& chunk)
					{
						return destination.consume(chunk);
					});
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

		private:
		   Source      source;
		   Destination destination;
	};
}
