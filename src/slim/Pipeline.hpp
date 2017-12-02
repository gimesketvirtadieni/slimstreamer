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

#include "slim/alsa/Source.hpp"
#include "slim/wave/Destination.hpp"


namespace slim
{
	class Pipeline
	{
		public:
			Pipeline(alsa::Source s, wave::Destination d)
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

			inline void processChunks(unsigned int maxChunks)
			{
				auto processing{true};

				for (unsigned int count{0}; count < maxChunks && processing; count++)
				{
					processing = source.supply([&](Chunk& chunk)
					{
						destination.consume(chunk);
					});
				}
			}

			inline void start()
			{
				return source.startProducing();
			}

			inline void stop(bool gracefully)
			{
				return source.stopProducing(gracefully);
			}

		private:
		   alsa::Source      source;
		   wave::Destination destination;
	};
}
