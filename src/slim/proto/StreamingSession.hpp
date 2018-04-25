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
#include <optional>
#include <sstream>  // std::stringstream
#include <string>

#include "slim/Chunk.hpp"
#include "slim/log/log.hpp"
#include "slim/util/AsyncWriter.hpp"
#include "slim/util/ExpandableBuffer.hpp"


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType, typename EncoderType>
		class StreamingSession
		{
			public:
				StreamingSession(std::reference_wrapper<ConnectionType> co, unsigned int ch, unsigned int sr, unsigned int bs, unsigned int bv, std::string id)
				: connection{co}
				, samplingRate{sr}
				, clientID{id}
				, encoder{ch, sr, bs, bv, std::ref<util::AsyncWriter>(connection), false}
				{
					LOG(DEBUG) << LABELS{"proto"} << "HTTP session object was created (id=" << this << ")";

					// creating response string
					std::stringstream ss;
					ss << "HTTP/1.1 200 OK\r\n"
					   << "Server: SlimStreamer ("
					   // TODO: provide version to the constructor
					   << VERSION
					   << ")\r\n"
					   << "Connection: close\r\n"
					   << "Content-Type: " << encoder.getMIME() << "\r\n"
					   << "\r\n";

					// sending response string
					connection.get().write(ss.str());
				}

				virtual ~StreamingSession()
				{
					LOG(DEBUG) << LABELS{"proto"} << "HTTP session object was deleted (id=" << this << ")";
				}

				StreamingSession(const StreamingSession&) = delete;             // non-copyable
				StreamingSession& operator=(const StreamingSession&) = delete;  // non-assignable
				StreamingSession(StreamingSession&& rhs) = delete;              // non-movable
				StreamingSession& operator=(StreamingSession&& rhs) = delete;   // non-movable-assignable

				inline auto getClientID()
				{
					return clientID;
				}

				inline auto getSamplingRate()
				{
					return samplingRate;
				}

				inline void onChunk(Chunk chunk)
				{
					if (samplingRate == chunk.getSamplingRate())
					{
						encoder.encode(chunk.getData(), chunk.getSize());
					}
					else
					{
						LOG(WARNING) << LABELS{"proto"} << "Skipping chunk transmition due to different sampling rate used by a client";
					}
				}

				inline void onRequest(unsigned char* buffer, std::size_t size)
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
				}

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
				std::reference_wrapper<ConnectionType> connection;
				unsigned int                           samplingRate;
				std::string                            clientID;
				EncoderType                            encoder;
		};
	}
}
