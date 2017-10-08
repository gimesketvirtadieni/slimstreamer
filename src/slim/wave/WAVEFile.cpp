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

#include <cstddef>

#include "slim/wave/WAVEFile.hpp"


namespace slim
{
	namespace wave
	{
		WAVEFile::WAVEFile(const char* f)
		: fileName{f}
		, outputFile{fileName, std::ios::binary}
		{
			write((const unsigned char*)"wav", 3);
		}


		WAVEFile::~WAVEFile()
		{
	        outputFile.close();
		}


		void WAVEFile::consume(Chunk& chunk)
		{
			write(chunk.getBuffer(), chunk.getSize());
		}


		void WAVEFile::write(const unsigned char* buffer, size_t size)
		{
		    if (outputFile.is_open())
		    {
			    for (unsigned int i = 0; i < size; i++)
		        {
					auto value = (char)buffer[i];
					outputFile.write(&value, sizeof(char));
		        }
		    }
		}
	}
}
