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
#include <vector>


namespace slim
{
	namespace util
	{
        template <typename ElementType>
        class RingBuffer
        {
            public:
                using size_type = std::size_t;

                typedef ElementType&       reference;
                typedef const ElementType& const_reference;
                typedef ElementType*       pointer;
                typedef const ElementType* const_pointer;
            
                RingBuffer() = delete;

                explicit RingBuffer(size_type c)
                : buffer{c}
                , capacity{c} {}

				~RingBuffer() = default;
				RingBuffer(const RingBuffer&) = delete;             // non-copyable
				RingBuffer& operator=(const RingBuffer&) = delete;  // non-assignable
				RingBuffer(RingBuffer&&) = default;
				RingBuffer& operator=(RingBuffer&&) = default;

                const_reference operator[](size_type index) const
                {
                    static const std::out_of_range ex("index out of range");
                    if (index < 0) throw ex;

                    if (size == capacity)
                    {
                        if (index >= capacity) throw ex;
                        return buffer[(front + index) % capacity];
                    }
                    else
                    {
                        if (index >= front) throw ex;
                        return buffer[index];
                    }
                }

                reference operator[](size_type index)
                {
                    return const_cast<reference>(static_cast<const RingBuffer<ElementType>&>(*this)[index]);
                }

                void pushBack(ElementType item)
                {
                    buffer[front++] = item;
                    if (front == capacity)
                    {
                        front = 0;
                    }
                }

                size_type getCapacity() const
                {
                    return capacity;
                }
                
                size_type getSize() const
                { 
                    return (size == capacity ? capacity : front);
                }

                bool isFull() const
                {
                    return size == capacity;
                }

            private:
                std::vector<ElementType> buffer;
                size_type                capacity{0};
                size_type                size{0};
                size_type                front{0};
                size_type                back{0};
        };
    }
}
