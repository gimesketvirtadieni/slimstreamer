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
#include <cstdint>

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
				outputFile.seekp(0, std::ios::end);
				auto dataLength = outputFile.tellp();

				// writting chunk2 size field
				// TODO: replace 44 with sizeof
				dataLength -= 44;
				outputFile.seekp(44 - sizeof(int32_t));
				outputFile.write(reinterpret_cast<const char*>(&dataLength), sizeof(int32_t));

				// writting chunk size field
				dataLength += 36;
				outputFile.seekp(4);
				outputFile.write(reinterpret_cast<const char*>(&dataLength), sizeof(int32_t));
		    }
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
			const unsigned char chunkID[]     = {0x52, 0x49, 0x46, 0x46};
			const unsigned char size[]        = {0x00, 0x00, 0x00, 0x00};
			const unsigned char format[]      = {0x57, 0x41, 0x56, 0x45};
			const unsigned char subchunk1ID[] = {0x66, 0x6D, 0x74, 0x20};
			const unsigned char size1[]       = {0x10, 0x00, 0x00, 0x00};
			const unsigned char format1[]     = {0x01, 0x00};  // PCM data = 0x01
			const unsigned char subchunk2ID[] = {0x64, 0x61, 0x74, 0x61};

			write(chunkID, sizeof(chunkID));
			write(size, sizeof(size));
			write(format, sizeof(format));

			write(subchunk1ID, sizeof(subchunk1ID));
			write(size1, sizeof(size1));
			write(format1, sizeof(format1));
			outputFile.write(reinterpret_cast<const char*>(&channels), sizeof(uint16_t));
			outputFile.write(reinterpret_cast<const char*>(&sampleRate), sizeof(uint32_t));
			outputFile.write(reinterpret_cast<const char*>(&byteRate), sizeof(uint32_t));
			outputFile.write(reinterpret_cast<const char*>(&bytesPerFrame), sizeof(uint16_t));
			outputFile.write(reinterpret_cast<const char*>(&bitsPerSample), sizeof(uint16_t));

			write(subchunk2ID, sizeof(subchunk2ID));
			write(size, sizeof(size));
		}
	}
}
