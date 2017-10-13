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
			writeHeader();
		}


		WAVEFile::~WAVEFile()
		{
			updateHeader();
	        outputFile.close();
		}


		void WAVEFile::consume(Chunk& chunk)
		{
			write(chunk.getBuffer(), chunk.getSize());
		}


		void WAVEFile::updateHeader()
		{
		    if (outputFile.is_open())
		    {
				outputFile.seekp(4);
		    }
			write((const unsigned char*)"wav3", 4);
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


		void WAVEFile::writeHeader()
		{
			const unsigned char chunkID[]    = {0x52, 0x49, 0x46, 0x46};
			const unsigned char size[]       = {0x00, 0x00, 0x00, 0x00};
			const unsigned char format[]     = {0x57, 0x41, 0x56, 0x45};
			const unsigned char subchunkID[] = {0x66, 0x6D, 0x74, 0x20};

			write(chunkID, sizeof(chunkID));
			write(size, sizeof(size));
			write(format, sizeof(format));
			write(subchunkID, sizeof(subchunkID));
		}
	}
}
