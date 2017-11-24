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

#include <cstddef>
#include <memory>


namespace slim
{
	class Chunk
	{
		public:
			Chunk()
			: size{0}
			, dataSize{0}
			, buffer{nullptr} {}

			// using Rule Of Zero
			~Chunk() = default;
			Chunk(const Chunk&) = delete;             // non-copyable
			Chunk& operator=(const Chunk&) = delete;  // non-assignable
			Chunk(Chunk&& rhs) = default;
			Chunk& operator=(Chunk&& rhs) = default;

			inline unsigned char* getBuffer() const
			{
				return buffer.get();
			}

			inline size_t getDataSize() const
			{
				return dataSize;
			}

			inline size_t getSize() const
			{
				return size;
			}

			inline void setDataSize(size_t s)
			{
				// TODO: handle case when s > buffer size
				dataSize = s;
			}

			inline void setSize(size_t s)
			{
				size     = s;
				dataSize = 0;

				// allocating buffer
				buffer = std::make_unique<unsigned char[]>(size);
			}

		private:
			size_t                           size;
			size_t                           dataSize;
			std::unique_ptr<unsigned char[]> buffer;
	};
}
