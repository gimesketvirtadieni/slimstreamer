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

				inline static auto isEnoughData(unsigned char* buffer, std::size_t size)
				{
					auto        result{false};
					std::size_t offset{4};

					// TODO: consider max length size
					if (size >= offset + sizeof(std::uint32_t))
					{
						std::uint32_t length{ntohl(*(std::uint32_t*)(buffer + offset))};
						result = (size >= (length + offset + sizeof(std::uint32_t)));
					}

					return result;
				}

			protected:
				Command() = default;
		};
	}
}
