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

#include "slim/Chunk.hpp"


namespace slim
{
	void Chunk::reset(size_t s)
	{
		size = s;

		// allocating buffer
		buffer = std::make_unique<unsigned char[]>(size);
	}


	unsigned char* Chunk::getBuffer() const
	{
		return buffer.get();
	}


	size_t Chunk::getSize() const
	{
		return size;
	}


	void Chunk::setSize(size_t s)
	{
		// TODO: handle case when s > buffer size
		size = s;
	}
}
