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
	struct Chunk
	{
		using BufferType = util::buffer::HeapBuffer<std::uint8_t>;

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

		bool             endOfStream{false};
		unsigned int     samplingRate{0};
		unsigned int     channels{0};
		unsigned int     bytesPerSample{0};
		BufferType       buffer{0};
		std::size_t      frames{0};
		util::BigInteger sequenceNumber{0};
		util::BigInteger capturedFrames{0};
		util::Timestamp  timestamp;
	};
}
