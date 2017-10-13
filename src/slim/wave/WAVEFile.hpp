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

#include <fstream>

#include "slim/Chunk.hpp"


namespace slim
{
	namespace wave
	{
		class WAVEFile
		{
			public:
				     WAVEFile(const char* fileName);
			        ~WAVEFile();
				void consume(Chunk& chunk);

			protected:
				void updateHeader();
				void write(const unsigned char* buffer, size_t size);
				void writeHeader();

			private:
				const char*   fileName;
				std::ofstream outputFile;
		};
	}
}
