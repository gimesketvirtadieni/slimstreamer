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
			               Chunk();
		                  ~Chunk();
			void           reset(unsigned long frames, unsigned int channels, unsigned int bitDepth);
			unsigned char* getBuffer() const;
			unsigned long  getFrames() const;
			size_t         getSize() const;
			void           setFrames(unsigned long frames);

		private:
			size_t                           size;
			unsigned long                    frames;
			std::unique_ptr<unsigned char[]> buffer;
	};
}
