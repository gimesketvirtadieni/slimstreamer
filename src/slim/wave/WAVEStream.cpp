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

#include <cstdint>

#include "slim/log/log.hpp"
#include "slim/wave/WAVEStream.hpp"


namespace slim
{
	namespace wave
	{
		void WAVEStream::write(std::string str)
		{
			write(str.c_str(), str.length());
		}


		void WAVEStream::write(const void* buffer, std::size_t size)
		{
			outputStreamPtr->write(reinterpret_cast<const char*>(buffer), size);
			outputStreamPtr->flush();
		}


		void WAVEStream::writeHeader(std::uint32_t size)
		{
			const char chunkID[]     = {0x52, 0x49, 0x46, 0x46};
			const char format[]      = {0x57, 0x41, 0x56, 0x45};
			const char subchunk1ID[] = {0x66, 0x6D, 0x74, 0x20};
			const char size1[]       = {0x10, 0x00, 0x00, 0x00};
			const char format1[]     = {0x01, 0x00};  // PCM data = 0x01
			const char subchunk2ID[] = {0x64, 0x61, 0x74, 0x61};

			write(chunkID, sizeof(chunkID));
			write(&size, sizeof(size));
			write(format, sizeof(format));
			write(subchunk1ID, sizeof(subchunk1ID));
			write(size1, sizeof(size1));
			write(format1, sizeof(format1));
   			write(&channels, sizeof(std::uint16_t));
			write(&sampleRate, sizeof(std::uint32_t));
			write(&byteRate, sizeof(std::uint32_t));
			write(&bytesPerFrame, sizeof(std::uint16_t));
			write(&bitsPerSample, sizeof(std::int16_t));
			write(subchunk2ID, sizeof(subchunk2ID));
			write(&size, sizeof(size));
		}
	}
}
