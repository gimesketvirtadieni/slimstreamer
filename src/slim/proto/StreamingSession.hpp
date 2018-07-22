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
#include <memory>
#include <optional>
#include <sstream>  // std::stringstream
#include <string>

#include "slim/Chunk.hpp"
#include "slim/EncoderBase.hpp"
#include "slim/log/log.hpp"
#include "slim/util/AsyncWriter.hpp"
#include "slim/util/ExpandableBuffer.hpp"


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType>
		class StreamingSession
		{
			public:
				StreamingSession(std::reference_wrapper<ConnectionType> co, std::unique_ptr<EncoderBase> e, std::string id)
				: connection{co}
				, encoderPtr{std::move(e)}
				, clientID{id}
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
					   << "Content-Type: " << encoderPtr->getMIME() << "\r\n"
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

				inline void onChunk(Chunk& chunk)
				{
					if (encoderPtr->getSamplingRate() == chunk.getSamplingRate())
					{
						encoderPtr->encode(chunk.getData(), chunk.getSize());
					}
					else
					{
						LOG(WARNING) << LABELS{"proto"} << "Closing HTTP connection due to different sampling rate used by a client (session rate=" << encoderPtr->getSamplingRate() << "; chunk rate=" << chunk.getSamplingRate() << ")";
						connection.get().stop();
					}
				}

				inline void onRequest(unsigned char* buffer, std::size_t size)
				{
					// TODO: make more strick validation
					std::string get{"GET"};
					std::string s{(char*)buffer, get.size()};
					if (get.compare(s))
					{
						// stopping this session due an error
						LOG(WARNING) << LABELS{"proto"} << "Wrong HTTP method provided";
						connection.get().stop();
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
				std::unique_ptr<EncoderBase>           encoderPtr;
				std::string                            clientID;
		};
	}
}
