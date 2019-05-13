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
#include <memory>


namespace slim
{
	namespace util
	{
        template <typename ElementType>
        class RingBuffer
        {
            public:
                using SizeType           = std::size_t;
                using ReferenceType      = ElementType&;
                using ConstReferenceType = const ElementType&;
            
                RingBuffer() = delete;

                inline explicit RingBuffer(SizeType c)
                : bufferPtr{std::make_unique<ElementType[]>(c)}
                , capacity{c} {}

				~RingBuffer() = default;
				RingBuffer(const RingBuffer&) = delete;             // non-copyable
				RingBuffer& operator=(const RingBuffer&) = delete;  // non-assignable
				RingBuffer(RingBuffer&&) = default;
				RingBuffer& operator=(RingBuffer&&) = default;

                inline ConstReferenceType operator[](const SizeType& i) const
                {
                    return bufferPtr.get()[index(head + i)];
                }

                inline ReferenceType operator[](const SizeType& i)
                {
                    return bufferPtr.get()[index(head + i)];
                }

                inline void clear()
                {
                    size = 0;
                }

                inline SizeType getCapacity() const
                {
                    return capacity;
                }
                
                inline SizeType getSize() const
                { 
                    return size;
                }

                inline bool isEmpty() const
                {
                    return size == 0;
                }

                inline bool isFull() const
                {
                    return size == capacity;
                }

                inline bool popBack()
                {
                    auto result{false};

                    if (!isEmpty())
                    {
                        size--;
                        result = true;
                    }

                    return result;
                }

                inline bool popFront()
                {
                    auto result{popBack()};

                    if (result)
                    {
                        head = index(head + 1);
                    }

                    return result;
                }

                inline SizeType pushBack(const ElementType& item)
                {
                    if (!isFull())
                    {
                        size++;
                    }
                    else
                    {
                        head = index(head + 1);
                    }

                    auto i{index(head + size - 1)};
                    bufferPtr.get()[i] = item;

                    return i;
                }

            protected:
                inline SizeType index(const SizeType& value) const
                {
                    return value < capacity ? value : value - capacity;
                }

            private:
                std::unique_ptr<ElementType[]> bufferPtr;
                SizeType                       capacity;
                SizeType                       size{0};
                SizeType                       head{0};
        };
    }
}
