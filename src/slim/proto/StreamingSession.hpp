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
				StreamingSession(ConnectionType& c)
				: connection(c)
				, outputStreamCallback{[&](auto* buffer, auto size) mutable
				{
					// TODO: work in progress
					auto& socket = connection.getNativeSocket();
					if (socket.is_open())
					{
						socket.send(asio::buffer(buffer, size));
					}

					return size;
				}}
				, waveStream{std::make_unique<std::ostream>(&outputStreamCallback), 2, 48000, 32}
				{
					LOG(INFO) << "HTTP session created";

					// TODO: work in progress
					waveStream.write("HTTP/1.1 200 OK\n");
					waveStream.write("Server: Logitech Media Server (7.9.1 - 1513400996)\n");
					waveStream.write("Connection: close\n");
					waveStream.write("Content-Type: audio/x-wave\n");
					waveStream.write("\r\n\r\n");
				}

				~StreamingSession()
				{
					LOG(INFO) << "HTTP session deleted";
				}

				StreamingSession(const StreamingSession&) = delete;             // non-copyable
				StreamingSession& operator=(const StreamingSession&) = delete;  // non-assignable
				StreamingSession(StreamingSession&& rhs) = delete;              // non-movable
				StreamingSession& operator=(StreamingSession&& rhs) = delete;   // non-movable-assignable

				inline void consume(Chunk& chunk)
				{
					waveStream.write(chunk.getBuffer(), chunk.getDataSize());
				}

				inline auto& getConnection()
				{
					return connection;
				}

				void onData(unsigned char* buffer, std::size_t receivedSize)
				{
					LOG(DEBUG) << "HTTP onData";

					for (unsigned int ii = 0; ii < receivedSize; ii++)
					{
						LOG(DEBUG) << buffer[ii];
					}
				}

			private:
				ConnectionType&      connection;
				OutputStreamCallback outputStreamCallback;
				wave::WAVEStream     waveStream;
		};
	}
}
