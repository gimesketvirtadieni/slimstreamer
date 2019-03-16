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
#include "slim/util/Buffer.hpp"


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
			Chunk(bool en, unsigned int sr, unsigned int ch, unsigned int bi)
			: endOfStream{en}
			, samplingRate{sr}
			, channels{ch}
			, bitsPerSample{bi} {}

			~Chunk() = default;
			Chunk(const Chunk& rhs) = delete;
			Chunk& operator=(const Chunk& rhs) = delete;
			Chunk(Chunk&& rhs) = delete;
			Chunk& operator=(Chunk&& rhs) = delete;

			template<typename FuncType>
			inline void fillWithData(FuncType func)
			{
				// TODO: introduce assert to make sure frames <= size / bytes per frame
				frames = func(buffer.getData(), buffer.getSize());
			}

			inline void flush()
			{
				frames = 0;
			}

			inline auto getBufferSize() const
			{
				return buffer.getSize();
			}

			inline auto getChannels() const
			{
				return channels;
			}

			inline auto* getData() const
			{
				return buffer.getData();
			}

			inline std::size_t getDataSize() const
			{
				return frames * channels * (bitsPerSample >> 3);
			}

			inline auto getFrames() const
			{
				return frames;
			}

			inline unsigned int getSamplingRate() const
			{
				return samplingRate;
			}

			inline bool isEndOfStream() const
			{
				return endOfStream;
			}

			inline void setBitsPerSample(unsigned int b)
			{
				bitsPerSample = b;
			}

			inline void setBufferSize(std::size_t c)
			{
				buffer = std::move(util::Buffer{c});

				// changing buffer size resets the whole content of this chunk, hence reseting amount of frames stored
				frames = 0;
			}

			inline void setChannels(unsigned int c)
			{
				channels = c;
			}

			inline void setEndOfStream(bool e)
			{
				endOfStream = e;
			}

			inline void setSamplingRate(unsigned int r)
			{
				samplingRate = r;
			}

		protected:
			Chunk() = default;

		private:
			bool         beginningOfStream{false};
			bool         endOfStream{false};
			unsigned int samplingRate{0};
			unsigned int channels{0};
			unsigned int bitsPerSample{0};
			std::size_t  frames{0};
			util::Buffer buffer{0};
		};
}
