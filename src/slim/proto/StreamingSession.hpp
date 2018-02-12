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
#include <optional>
#include <string>

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
				StreamingSession(ConnectionType& co, unsigned int channels, unsigned int sr, unsigned int bitePerSample)
				: connection{co}
				, outputStreamCallback{[&](auto* buffer, auto size) mutable
				{
					return connection.send(buffer, size);
				}}
				, samplingRate{sr}
				, waveStream{std::make_unique<std::ostream>(&outputStreamCallback), channels, samplingRate, bitePerSample}
				{
					LOG(DEBUG) << LABELS{"proto"} << "HTTP session object was created (id=" << this << ")";

					// sending HTTP response with the headers
					waveStream.write("HTTP/1.1 200 OK\r\n");
					waveStream.write("Server: SlimStreamer (");
					// TODO: provide version to the constructor
					waveStream.write(VERSION);
					waveStream.write(")\r\n");
					waveStream.write("Connection: close\r\n");
					waveStream.write("Content-Type: audio/x-wave\r\n");
					waveStream.write("\r\n");
				}

			   ~StreamingSession()
				{
					LOG(DEBUG) << LABELS{"proto"} << "HTTP session object was deleted (id=" << this << ")";
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
						LOG(WARNING) << LABELS{"proto"} << "Skipping chunk transmition due to different sampling rate used by a client";
					}
				}

				inline auto getClientID()
				{
					return clientID;
				}

				inline auto getSamplingRate()
				{
					return samplingRate;
				}

				void onRequest(unsigned char* buffer, std::size_t size)
				{
					// LOG(DEBUG) << LABELS{"proto"} << "HTTP onRequest";

					//for (unsigned int i = 0; i < size; i++)
					//{
					//	LOG(DEBUG) << buffer[i];
					//}

					// TODO: make more strick validation
					std::string get{"GET"};
					std::string s{(char*)buffer, get.size()};
					if (get.compare(s))
					{
						throw slim::Exception("Wrong method provided");
					}

					clientID = parseClientID({(char*)buffer, size});
					if (!clientID.has_value())
					{
						throw slim::Exception("Missing client ID in HTTP request");
					}

					LOG(INFO) << LABELS{"proto"} << "Client ID was parsed from HTTP request (clientID=" << clientID.value() << ")";
				}

			protected:
				static auto parseClientID(std::string header)
				{
					auto result{std::optional<std::string>{std::nullopt}};
					auto separator{std::string{"="}};

					auto index = header.find(separator);
					if ( index != std::string::npos)
					{
						result = std::string{header.c_str() + index + separator.length(), header.length() - index - separator.length()};
					}

					return result;
				}

			private:
				ConnectionType&            connection;
				std::optional<std::string> clientID;
				OutputStreamCallback       outputStreamCallback;
				unsigned int               samplingRate;
				wave::WAVEStream           waveStream;
		};
	}
}
