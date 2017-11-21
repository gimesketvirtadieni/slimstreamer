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
#include <string>

#include "slim/Chunk.hpp"


namespace slim
{
	namespace wave
	{
		class WAVEFile
		{
			public:
				explicit WAVEFile(const char* f, unsigned int c, unsigned int s, int b)
				: fileName{f}
				, channels{c}
				, sampleRate{s}
				, bitsPerSample{b}
				, outputFile{fileName, std::ios::binary}
				{
					writeHeader();
				}

				// there is a need for a custom destructor so Rule Of Zero cannot be used
				// Instead of The Rule of The Big Four (and a half) the following approach is used: http://scottmeyers.blogspot.dk/2014/06/the-drawbacks-of-implementing-move.html
				~WAVEFile()
				{
					if (!empty)
					{
						updateHeader();
					}
				}

				WAVEFile(const WAVEFile&) = delete;             // non-copyable
				WAVEFile& operator=(const WAVEFile&) = delete;  // non-assignable

				WAVEFile(WAVEFile&& rhs)
				: fileName{std::move(rhs.fileName)}
				, channels{std::move(rhs.channels)}
				, sampleRate{std::move(rhs.sampleRate)}
				, bitsPerSample{std::move(rhs.bitsPerSample)}
				, byteRate{std::move(rhs.byteRate)}
				, bytesPerFrame{std::move(rhs.bytesPerFrame)}
				, outputFile{std::move(rhs.outputFile)}
				{
					rhs.empty = true;
				}

				WAVEFile& operator=(WAVEFile&& rhs)
				{
					using std::swap;

					// any resources should be released for this object here because it will take over resources from rhs object
					if (!empty)
					{
						updateHeader();
					}
					rhs.empty = true;

					swap(fileName, rhs.fileName);
					swap(channels, rhs.channels);
					swap(sampleRate, rhs.sampleRate);
					swap(bitsPerSample, rhs.bitsPerSample);
					swap(byteRate, rhs.byteRate);
					swap(bytesPerFrame, rhs.bytesPerFrame);
					swap(outputFile, rhs.outputFile);

					return *this;
				}

				void consume(Chunk& chunk);

			protected:
				void updateHeader();
				void write(const unsigned char* buffer, size_t size);
				void writeHeader();

			private:
				// TODO: empty attribute should be refactored to a separate class
				bool          empty         = false;
				std::string   fileName;
				unsigned int  channels;
				unsigned int  sampleRate;
				int           bitsPerSample;
				unsigned int  byteRate      = sampleRate * channels * (bitsPerSample >> 3);
				unsigned int  bytesPerFrame = channels * (bitsPerSample >> 3);
				std::ofstream outputFile;
		};
	}
}
