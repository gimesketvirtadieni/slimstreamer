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

#include <cstddef>   // std::size_t

#include "slim/util/BigInteger.hpp"
#include "slim/util/buffer/HeapBuffer.hpp"
#include "slim/util/Timestamp.hpp"


namespace slim
{
	namespace util
	{
		template<typename T>
		class RealTimeQueue;
	}

	class Chunk
	{
		public:
			using BufferType = util::buffer::HeapBuffer<std::uint8_t>;

			Chunk(bool en, unsigned int sr, unsigned int ch, unsigned int bi)
			: endOfStream{en}
			, samplingRate{sr}
			, channels{ch}
			, bitsPerSample{bi} {}

			inline void allocateBuffer(std::size_t s)
			{
				if (buffer.getSize() != s)
				{
					buffer = std::move(BufferType{s});
				}
				clear();
			}

			inline void clear()
			{
				frames = 0;
				capturedFrames = 0;
			}

			inline auto getBufferSize() const
			{
				return buffer.getSize();
			}

			inline auto getBytesPerFrame() const
			{
				return channels * (bitsPerSample >> 3);
			}

			inline auto getCapturedFrames() const
			{
				return capturedFrames;
			}

			inline auto getChannels() const
			{
				return channels;
			}

			inline auto* getData() const
			{
				return buffer.getData();
			}

			inline auto getFrames() const
			{
				return frames;
			}

			inline unsigned int getSamplingRate() const
			{
				return samplingRate;
			}

			inline auto getTimestamp() const
			{
				return timestamp;
			}

			inline auto isEndOfStream() const
			{
				return endOfStream;
			}

			inline void setBitsPerSample(unsigned int b)
			{
				bitsPerSample = b;
			}

			inline void setCapturedFrames(util::BigInteger f)
			{
				capturedFrames = f;
			}

			inline void setChannels(unsigned int c)
			{
				channels = c;
			}

			inline void setEndOfStream(bool e)
			{
				endOfStream = e;
			}

			inline void setFrames(std::size_t f)
			{
				frames = f;
			}

			inline void setSamplingRate(unsigned int r)
			{
				samplingRate = r;
			}

			inline void setTimestamp(util::Timestamp t)
			{
				timestamp = t;
			}

		protected:
			friend util::RealTimeQueue<Chunk>;
			Chunk() = default;

		private:
			bool             endOfStream{false};
			unsigned int     samplingRate{0};
			unsigned int     channels{0};
			unsigned int     bitsPerSample{0};
			BufferType       buffer{0};
			std::size_t      frames{0};
			util::BigInteger capturedFrames{0};
			util::Timestamp  timestamp;
		};
}
