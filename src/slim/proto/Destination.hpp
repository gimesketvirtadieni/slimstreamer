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
#include "slim/log/log.hpp"
#include "slim/proto/Streamer.hpp"


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType>
		class Destination
		{
			public:
				Destination(Streamer<ConnectionType>& s, unsigned int sr)
				: streamer{s}
				, samplingRate{sr} {}

				// using Rule Of Zero
				~Destination() = default;
				Destination(const Destination&) = delete;
				Destination& operator=(const Destination&) = delete;
				Destination(Destination&& rhs) = default;
				Destination& operator=(Destination&& rhs) = default;

				inline void consume(Chunk& chunk)
				{
					streamer.onChunk(chunk, samplingRate);
				}

			private:
				Streamer<ConnectionType>& streamer;
				unsigned int              samplingRate;
		};
	}
}
