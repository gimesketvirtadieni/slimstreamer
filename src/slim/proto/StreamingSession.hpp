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

#include <functional>
#include <iostream>

#include "slim/log/log.hpp"
#include "slim/util/OutputStreamCallback.hpp"
#include "slim/wave/WAVEStream.hpp"


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType>
		class StreamingSession
		{
			using OutputStreamCallback = util::OutputStreamCallback<std::function<std::streamsize(const char*, std::streamsize)>>;

			public:
				StreamingSession(ConnectionType& c, unsigned int channels, unsigned int sr, unsigned int bitePerSample)
				: connection(c)
				, outputStreamCallback{[&](auto* buffer, auto size) mutable
				{
					return connection.send(buffer, size);
				}}
				, samplingRate{sr}
				, waveStream{std::make_unique<std::ostream>(&outputStreamCallback), channels, samplingRate, bitePerSample}
				{
					LOG(INFO) << "HTTP session created";

					// sending HTTP response with the headers
					waveStream.write("HTTP/1.1 200 OK\r\n");
					waveStream.write("Server: SlimStreamer (0.0.1)\r\n");
					waveStream.write("Connection: close\r\n");
					waveStream.write("Content-Type: audio/x-wave\r\n");
					waveStream.write("\r\n");
				}

				~StreamingSession()
				{
					LOG(INFO) << "HTTP session deleted";
				}

				StreamingSession(const StreamingSession&) = delete;             // non-copyable
				StreamingSession& operator=(const StreamingSession&) = delete;  // non-assignable
				StreamingSession(StreamingSession&& rhs) = delete;              // non-movable
				StreamingSession& operator=(StreamingSession&& rhs) = delete;   // non-movable-assignable

				inline void onChunk(Chunk& chunk, unsigned int sr)
				{
					if (samplingRate == sr)
					{
						waveStream.write(chunk.getBuffer(), chunk.getDataSize());
					}
					else
					{
						// TODO: handle properly; work in progress
						LOG(DEBUG) << "incorrect rate session rate=" << samplingRate << " data chunk rate=" << sr;
					}
				}

				inline auto& getConnection()
				{
					return connection;
				}

				inline auto getSamplingRate()
				{
					return samplingRate;
				}

				void onRequest(unsigned char* buffer, std::size_t receivedSize)
				{
					LOG(DEBUG) << "HTTP onData";

					//for (unsigned int ii = 0; ii < receivedSize; ii++)
					//{
					//	LOG(DEBUG) << buffer[ii];
					//}
				}

			private:
				ConnectionType&      connection;
				OutputStreamCallback outputStreamCallback;
				unsigned int         samplingRate;
				wave::WAVEStream     waveStream;
		};
	}
}
