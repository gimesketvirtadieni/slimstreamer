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

#include "slim/util/ExpandableBuffer.hpp"


namespace slim
{
	namespace util
	{
		template<typename T>
		class RealTimeQueue;
	}

	class Chunk
	{
		friend util::RealTimeQueue<Chunk>;

		public:
			Chunk(unsigned int sr, unsigned int c, unsigned int b)
			: samplingRate{sr}
			, channels{c}
			, bitsPerSample{b} {}

			~Chunk() = default;
			Chunk(const Chunk& rhs) = delete;
			Chunk& operator=(const Chunk& rhs) = delete;
			Chunk(Chunk&& rhs) = delete;
			Chunk& operator=(Chunk&& rhs) = delete;

			inline auto getChannels() const
			{
				return channels;
			}

			inline auto* getData() const
			{
				return buffer.data();
			}

			inline auto getDurationMicroseconds() const
			{
				unsigned long long result{0};

				if (samplingRate)
				{
					result = std::chrono::microseconds{getFrames() * 1000000 / samplingRate}.count();
				}

				return result;
			}

			inline std::size_t getFrames() const
			{
				return getSize() / (channels * (bitsPerSample >> 3));
			}

			inline unsigned int getSamplingRate() const
			{
				return samplingRate;
			}

			inline std::size_t getSize() const
			{
				return buffer.size();
			}

			inline void setBitsPerSample(unsigned int b)
			{
				bitsPerSample = b;
			}

			inline void setCapacity(std::size_t c)
			{
				buffer.capacity(c);
			}

			inline void setChannels(unsigned int c)
			{
				channels = c;
			}

			inline void setSamplingRate(unsigned int r)
			{
				samplingRate = r;
			}

			inline void setSize(std::size_t s)
			{
				buffer.size(s);
			}

		protected:
			Chunk() {}

		private:
			unsigned int           samplingRate{0};
			unsigned int           channels{0};
			unsigned int           bitsPerSample{0};
			util::ExpandableBuffer buffer;
		};
}
