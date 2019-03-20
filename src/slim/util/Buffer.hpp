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

#include <cstddef>   // std::size_t
#include <cstdint>   // std::u..._t types
#include <cstring>   // std::memcpy
#include <memory>    // std::unique_ptr
#include <stdexcept> // std::out_of_range, std::invalid_argument


namespace slim
{
    namespace util
    {
        // TODO: implement asserts
        class Buffer
        {
            public:
                using size_type = std::size_t;
                using byte_type = std::uint8_t;

                inline Buffer() : Buffer{0} {}

                inline Buffer(size_type s)
                : data{s > 0 ? std::make_unique<byte_type[]>(s) : nullptr}
                , size{s > 0 ? s : 0} {}

                inline Buffer(const void* d, const size_type s)
                : Buffer{s}
                {
                    addData(d, s);
                }

				// there is a need for a custom destructor so Rule Of Zero cannot be used: http://scottmeyers.blogspot.dk/2014/06/the-drawbacks-of-implementing-move.html
				~Buffer() = default;
                Buffer(const Buffer& rhs) = delete;
                Buffer& operator=(const Buffer& rhs) = delete;
                Buffer(Buffer&& rhs) = default;
                Buffer& operator=(Buffer&& rhs) = default;

				inline byte_type& at(size_type i) const
                {
                    if (i >= size)
                    {
                        throw std::out_of_range{"Index is out of range"};
                    }

                    return getData()[i];
                }

				inline void addData(const void* d, const size_type s)
				{
                    if (0 <= s && s + dataSize <= size)
                    {
                        std::memcpy(data.get() + dataSize, d, s);
                        dataSize += s;
                    }
				}

				inline void clear()
				{
                    setDataSize(0);
				}

				inline byte_type* getData() const
                {
                    return data.get();
                }

				inline size_type getDataSize() const
				{
					return dataSize;
				}

				inline size_type getSize() const
				{
					return size;
				}

				inline bool isEmpty() const
				{
					return dataSize == 0;
				}

				inline void setDataSize(size_type s)
				{
                    if (s <= size)
                    {
                        dataSize = s;
                    }
				}

				inline byte_type& operator[] (size_type i) const
                {
                    // No bounds checking is performed: https://en.cppreference.com/w/cpp/container/array/operator_at
                    return getData()[i];
                }

			private:
				std::unique_ptr<byte_type[]> data;
				size_type                    size{0};
				size_type                    dataSize{0};
		};

		inline bool operator== (const Buffer& a, const Buffer& b)
		{
            return a.getSize() == b.getSize() && std::memcmp (a.getData(), b.getData(), a.getSize()) == 0;
		}

		inline bool operator!= (const Buffer& a, const Buffer& b)
		{
            return !(a == b);
		}
	}
}
