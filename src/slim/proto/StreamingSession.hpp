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

#include "slim/log/log.hpp"


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType>
		class StreamingSession
		{
			public:
				StreamingSession(ConnectionType& c)
				: connection(c)
				{
					LOG(INFO) << "HTTP session created";
				}

				~StreamingSession()
				{
					LOG(INFO) << "HTTP session deleted";
				}
				StreamingSession(const StreamingSession&) = delete;             // non-copyable
				StreamingSession& operator=(const StreamingSession&) = delete;  // non-assignable
				StreamingSession(StreamingSession&& rhs) = delete;              // non-movable
				StreamingSession& operator=(StreamingSession&& rhs) = delete;   // non-movable-assignable

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
				ConnectionType& connection;
		};
	}
}
