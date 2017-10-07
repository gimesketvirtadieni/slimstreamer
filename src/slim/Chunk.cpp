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
