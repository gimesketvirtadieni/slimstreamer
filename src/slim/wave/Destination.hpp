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

#include "slim/Chunk.hpp"
#include "slim/wave/WAVEFile.hpp"


namespace slim
{
	namespace wave
	{
		class Destination
		{
			public:
				explicit Destination(std::string fileName, unsigned int channels, unsigned int sampleRate, int bitsPerSample)
				: waveFile{fileName, channels, sampleRate, bitsPerSample} {}

				// using Rule Of Zero
				~Destination() = default;
				Destination(const Destination&) = delete;
				Destination& operator=(const Destination&) = delete;
				Destination(Destination&& rhs) = default;
				Destination& operator=(Destination&& rhs) = default;

				void consume(Chunk& chunk);

			private:
				WAVEFile waveFile;
		};
	}
}
