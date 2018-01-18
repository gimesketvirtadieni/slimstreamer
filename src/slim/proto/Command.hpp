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

#include <cstddef>  // std::size_t


namespace slim
{
	namespace proto
	{
		template<typename CommandType>
		class Command
		{
			public:
				// using Rule Of Zero
				virtual ~Command() = default;
				Command(const Command&) = default;
				Command& operator=(const Command&) = default;
				Command(Command&& rhs) = default;
				Command& operator=(Command&& rhs) = default;

				virtual CommandType* getBuffer() = 0;
				virtual std::size_t  getSize()   = 0;

			protected:
				Command() = default;
		};
	}
}
