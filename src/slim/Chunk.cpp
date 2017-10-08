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
	Chunk::Chunk() : size(0), frames(0), buffer(nullptr) {}


	Chunk::~Chunk() {}


	void Chunk::reset(unsigned long f, unsigned int channels, unsigned int bitDepth)
	{
		frames = f;
		size   = frames * channels * (bitDepth >> 3);

		// allocating buffer
		buffer = std::make_unique<unsigned char[]>(size);
	}


	unsigned char* Chunk::getBuffer() const
	{
		return buffer.get();
	}


	unsigned long Chunk::getFrames() const
	{
		return frames;
	}


	size_t Chunk::getSize() const
	{
		return size;
	}


	void Chunk::setFrames(unsigned long f)
	{
		this->frames = f;
	}
}
