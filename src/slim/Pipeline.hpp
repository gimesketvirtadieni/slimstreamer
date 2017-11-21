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

#include "slim/alsa/Source.hpp"
#include "slim/wave/WAVEFile.hpp"


namespace slim
{
	class Pipeline
	{
		public:
			Pipeline(alsa::Source s, const char* fileName, unsigned int channels, unsigned int sampleRate, int bitsPerSample)
			: source{std::move(s)}
			, destination{fileName, channels, sampleRate, bitsPerSample} {}

			// using Rule Of Zero
		   ~Pipeline() = default;
			Pipeline(const Pipeline&) = delete;             // non-copyable
			Pipeline& operator=(const Pipeline&) = delete;  // non-assignable
			Pipeline(Pipeline&& rhs) = default;
			Pipeline& operator=(Pipeline&& rhs) = default;

			auto& getDestination()
			{
				return destination;
			}

			auto& getSource()
			{
				return source;
			}

		private:
		   alsa::Source   source;
		   wave::WAVEFile destination;
	};
}
