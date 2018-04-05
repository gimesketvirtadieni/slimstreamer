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

#include <cstddef>   // std::size_t
#include <cstdint>   // std::int..._t
#include <string>

#include "slim/StreamWriter.hpp"


namespace slim
{
	namespace wave
	{
		class Encoder
		{
			public:
				explicit Encoder(StreamWriter* w, unsigned int c, unsigned int s, unsigned int b)
				: writerPtr{w}
				, channels{c}
				, sampleRate{s}
				, bitsPerSample{b}
				, bytesPerFrame{channels * (bitsPerSample >> 3)}
				, byteRate{sampleRate * bytesPerFrame} {}

			   ~Encoder() = default;
				Encoder(const Encoder&) = delete;             // non-copyable
				Encoder& operator=(const Encoder&) = delete;  // non-assignable
				Encoder(Encoder&&) = delete;                  // non-movable
				Encoder& operator=(Encoder&&) = delete;       // non-assign-movable

				auto encode(unsigned char* data, const std::size_t size)
				{
					// TODO: error handling
					return writerPtr->write(data, size);
				}

				auto getBytesEncoded()
				{
					return writerPtr->getBytesWritten();
				}

				auto getMIME()
				{
					return std::string{"audio/x-wave"};
				}

				auto writeHeader(std::uint32_t size = 0)
				{
					const char chunkID[]     = {0x52, 0x49, 0x46, 0x46};
					const char format[]      = {0x57, 0x41, 0x56, 0x45};
					const char subchunk1ID[] = {0x66, 0x6D, 0x74, 0x20};
					const char size1[]       = {0x10, 0x00, 0x00, 0x00};
					const char format1[]     = {0x01, 0x00};  // PCM data = 0x01
					const char subchunk2ID[] = {0x64, 0x61, 0x74, 0x61};

					// creating header string
					std::stringstream ss;
					ss.write(chunkID, sizeof(chunkID));
					ss.write((const char*)&size, sizeof(size));
					ss.write(format, sizeof(format));
					ss.write(subchunk1ID, sizeof(subchunk1ID));
					ss.write(size1, sizeof(size1));
					ss.write(format1, sizeof(format1));
					ss.write((const char*)&channels, sizeof(std::uint16_t));
					ss.write((const char*)&sampleRate, sizeof(std::uint32_t));
					ss.write((const char*)&byteRate, sizeof(std::uint32_t));
					ss.write((const char*)&bytesPerFrame, sizeof(std::uint16_t));
					ss.write((const char*)&bitsPerSample, sizeof(std::int16_t));
					ss.write(subchunk2ID, sizeof(subchunk2ID));
					ss.write((const char*)&size, sizeof(size));

					// saving response string in this object; required for async transfer
					header = ss.str();

					// seeking to the beginning
					writerPtr->rewind(0);
					writerPtr->writeAsync(header);
				}

			private:
				StreamWriter* writerPtr;
				unsigned int  channels;
				unsigned int  sampleRate;
				unsigned int  bitsPerSample;
				unsigned int  bytesPerFrame;
				unsigned int  byteRate;
				std::string   header;
		};
	}
}
