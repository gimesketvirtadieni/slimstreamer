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
#include <functional>
#include <string>

#include "slim/EncoderBase.hpp"
#include "slim/log/log.hpp"
#include "slim/util/AsyncWriter.hpp"
#include "slim/util/BufferedAsyncWriter.hpp"


namespace slim
{
	namespace wave
	{
		class Encoder : public EncoderBase
		{
			public:
				explicit Encoder(unsigned int c, unsigned int s, unsigned int bs, unsigned int bv, std::reference_wrapper<util::AsyncWriter> w, bool h)
				: channels{c}
				, sampleRate{s}
				, bitsPerSample{bs}
				, bitsPerValue{bv}
				, bufferedWriter{w}
				, headerRequired{h}
				{
					if (headerRequired)
					{
						writeHeader();
					}
				}

				virtual ~Encoder()
				{
					if (headerRequired)
					{
						writeHeader(bytesWritten);
					}
				}

				Encoder(const Encoder&) = delete;             // non-copyable
				Encoder& operator=(const Encoder&) = delete;  // non-assignable
				Encoder(Encoder&&) = delete;                  // non-movable
				Encoder& operator=(Encoder&&) = delete;       // non-assign-movable

				virtual void encode(unsigned char* data, const std::size_t size) override
				{
					// do not feed encoder with more data if there is no room in transfer buffer
					if (bufferedWriter.isBufferAvailable())
					{
						bufferedWriter.writeAsync(data, size, [&](auto error, auto written)
						{
							if (!error)
							{
								bytesWritten += size;
							}
							else
							{
								LOG(ERROR) << LABELS{"wave"} << "Error while transferring encoded data: " << error.message();
							}
						});
					}
					else
					{
						LOG(WARNING) << LABELS{"flac"} << "Transfer buffer is full - skipping PCM chunk";
					}
				}

				auto getMIME()
				{
					return std::string{"audio/x-wave"};
				}

			protected:
				void writeHeader(std::size_t s = 0)
				{
					auto               size{static_cast<std::uint32_t>(s)};
					const unsigned int bytesPerFrame{channels * (bitsPerSample >> 3)};
					const unsigned int byteRate{sampleRate * bytesPerFrame};
					const char         chunkID[]     = {0x52, 0x49, 0x46, 0x46};
					const char         format[]      = {0x57, 0x41, 0x56, 0x45};
					const char         subchunk1ID[] = {0x66, 0x6D, 0x74, 0x20};
					const char         size1[]       = {0x10, 0x00, 0x00, 0x00};
					const char         format1[]     = {0x01, 0x00};  // PCM data = 0x01
					const char         subchunk2ID[] = {0x64, 0x61, 0x74, 0x61};

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

					// seeking to the beginning
					bufferedWriter.rewind(0);

					// no need to keep string to be sent as BufferedWriter uses its own buffer for async write
					bufferedWriter.writeAsync(ss.str(), [&](auto error, auto written)
					{
						if (!error)
						{
							bytesWritten += written;
						}
					});
				}

			private:
				unsigned int                  channels;
				unsigned int                  sampleRate;
				unsigned int                  bitsPerSample;
				unsigned int                  bitsPerValue;
				// TODO: parametrize
				util::BufferedAsyncWriter<10>      bufferedWriter;
				bool                               headerRequired;
				std::size_t                        bytesWritten{0};
		};
	}
}
