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

#include <cstdint>  // std::int..._t
#include <iostream>
#include <memory>


namespace slim
{
	namespace wave
	{
		class WAVEFile
		{
			public:
				explicit WAVEFile(std::unique_ptr<std::ostream> os, unsigned int c, unsigned int s, int b)
				: outputStreamPtr{std::move(os)}
				, channels{c}
				, sampleRate{s}
				, bitsPerSample{b}
				, bytesPerFrame{channels * (bitsPerSample >> 3)}
				, byteRate{sampleRate * bytesPerFrame} {}

				// using Rule Of Zero
			   ~WAVEFile() = default;
				WAVEFile(const WAVEFile&) = delete;             // non-copyable
				WAVEFile& operator=(const WAVEFile&) = delete;  // non-assignable
				WAVEFile(WAVEFile&&) = default;
				WAVEFile& operator=(WAVEFile&&) = default;

				auto& getOutputStream()
				{
					return *outputStreamPtr;
				}

				void write(std::string str);
				void write(const void* buffer, std::size_t size);
				void writeHeader(std::uint32_t size = 0);

			private:
				std::unique_ptr<std::ostream> outputStreamPtr;
				unsigned int                  channels;
				unsigned int                  sampleRate;
				int                           bitsPerSample;
				unsigned int                  bytesPerFrame;
				unsigned int                  byteRate;
		};
	}
}
