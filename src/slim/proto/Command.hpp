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
		enum class CommandSelection : char
		{
			Start   = 's',
			Stop    = 'q',
			Time    = 't',
			Pause   = 'p',
			Unpause = 'u',
		};

		enum class FormatSelection
		{
			PCM,
			FLAC,
		};

		template<typename CommandType>
		class Command
		{
			public:

				union AAA
				{
					std::uint8_t  ss[4];
					std::uint32_t nn;
				};

 				// using Rule Of Zero
				virtual ~Command() = default;
				Command(const Command&) = default;
				Command& operator=(const Command&) = default;
				Command(Command&& rhs) = default;
				Command& operator=(Command&& rhs) = default;

				virtual CommandType* getBuffer() = 0;
				virtual std::size_t  getSize()   = 0;

				template <typename BufferType>
				inline static auto isEnoughData2(const BufferType& buffer)
				{
					auto result{false};
					auto offset{typename BufferType::IndexType{4}};

					// TODO: consider max length size
					if (offset + sizeof(std::uint32_t) <= buffer.getSize())
					{
						AAA aaa;
						aaa.ss[0] = buffer[offset + 0];
						aaa.ss[1] = buffer[offset + 1];
						aaa.ss[2] = buffer[offset + 2];
						aaa.ss[3] = buffer[offset + 3];

						// there is enough data in the buffer if provided length is less of equal of buffer size
						result = (ntohl(aaa.nn) + offset + sizeof(std::uint32_t) <= buffer.getSize());
					}

					return result;
				}

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
