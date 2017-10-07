#pragma once

#include <cstddef>
#include <memory>

#include "alsa/Parameters.hpp"


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
