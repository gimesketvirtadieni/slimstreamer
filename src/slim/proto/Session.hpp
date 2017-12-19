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


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType>
		class Session
		{
			public:
				Session(ConnectionType& c)
				: connection(c) {}

				// using Rule Of Zero
				~Session() = default;
				Session(const Session&) = delete;             // non-copyable
				Session& operator=(const Session&) = delete;  // non-assignable
				Session(Session&& rhs) = default;
				Session& operator=(Session&& rhs) = default;

			private:
				ConnectionType& connection;
		};
	}
}
