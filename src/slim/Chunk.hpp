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

#include <functional>

#include "slim/util/ExpandableBuffer.hpp"


namespace slim
{
	class Chunk
	{
		public:
			Chunk(std::reference_wrapper<util::ExpandableBuffer> br, unsigned int sr, unsigned int c, unsigned int b)
			: buffer{br}
			, samplingRate{sr}
			, channels{c}
			, bitsPerSample{b} {}

			// using Rule Of Zero
		   ~Chunk() = default;
			Chunk(const Chunk& rhs) = default;
			Chunk& operator=(const Chunk& rhs) = default;
			Chunk(Chunk&& rhs) = default;
			Chunk& operator=(Chunk&& rhs) = default;

			inline auto getChannels()
			{
				return channels;
			}

			inline auto* getData()
			{
				return buffer.get().data();
			}

			inline std::size_t getFrames()
			{
				return getSize() / (channels * (bitsPerSample >> 3));
			}

			inline auto getSamplingRate()
			{
				return samplingRate;
			}

			inline std::size_t getSize()
			{
				return buffer.get().size();
			}

		private:
			std::reference_wrapper<util::ExpandableBuffer> buffer;
			unsigned int                                   samplingRate;
			unsigned int                                   channels;
			unsigned int                                   bitsPerSample;
		};
}
