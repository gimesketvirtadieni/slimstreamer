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
#include <memory>
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
					unsigned char chunkID[]     = {0x52, 0x49, 0x46, 0x46};
					unsigned char format[]      = {0x57, 0x41, 0x56, 0x45};
					unsigned char subchunk1ID[] = {0x66, 0x6D, 0x74, 0x20};
					unsigned char size1[]       = {0x10, 0x00, 0x00, 0x00};
					unsigned char format1[]     = {0x01, 0x00};  // PCM data = 0x01
					unsigned char subchunk2ID[] = {0x64, 0x61, 0x74, 0x61};

					// seeking to the beginning
					writerPtr->rewind(0);

					// writting header itself
					writerPtr->write(chunkID, sizeof(chunkID));
					writerPtr->write(&size, sizeof(size));
					writerPtr->write(format, sizeof(format));
					writerPtr->write(subchunk1ID, sizeof(subchunk1ID));
					writerPtr->write(size1, sizeof(size1));
					writerPtr->write(format1, sizeof(format1));
					writerPtr->write(&channels, sizeof(std::uint16_t));
					writerPtr->write(&sampleRate, sizeof(std::uint32_t));
					writerPtr->write(&byteRate, sizeof(std::uint32_t));
					writerPtr->write(&bytesPerFrame, sizeof(std::uint16_t));
					writerPtr->write(&bitsPerSample, sizeof(std::int16_t));
					writerPtr->write(subchunk2ID, sizeof(subchunk2ID));
					writerPtr->write(&size, sizeof(size));
				}

			private:
				StreamWriter* writerPtr;
				unsigned int  channels;
				unsigned int  sampleRate;
				unsigned int  bitsPerSample;
				unsigned int  bytesPerFrame;
				unsigned int  byteRate;
		};
	}
}
