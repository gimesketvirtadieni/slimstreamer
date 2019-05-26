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

#include "slim/util/buffer/ArrayBuffer.hpp"


namespace slim
{
	namespace util
	{
        template
        <
            typename ElementType,
            template <typename> class StorageType
        >
        class RingBufferAccessPolicy : public ArrayAccessPolicy<ElementType, StorageType>
        {
            public:
                inline explicit RingBufferAccessPolicy(StorageType<ElementType>& s)
                : ArrayAccessPolicy<ElementType, StorageType>{s} {}

                inline void clear()
                {
                    size = 0;
                }

                inline auto getSize() const
                { 
                    return size;
                }

                inline auto isEmpty() const
                {
                    return size == 0;
                }

                inline auto isFull() const
                {
                    return size == this->storage.getCapacity();
                }

                inline auto popBack()
                {
                    auto result{false};

                    if (!this->isEmpty())
                    {
                        size--;
                        result = true;
                    }

                    return result;
                }

                inline auto popFront()
                {
                    auto result{popBack()};

                    if (result)
                    {
                        head = normalizeIndex(head + 1);
                    }

                    return result;
                }

                inline auto pushBack(const ElementType& item)
                {
                    auto i{normalizeIndex(head + size)};
                    this->storage.getElement(i) = std::move(item);

                    if (!isFull())
                    {
                        size++;
                    }
                    else
                    {
                        head = normalizeIndex(head + 1);
                    }

                    return i;
                }

            protected:
                inline auto normalizeIndex(const typename ArrayAccessPolicy<ElementType, StorageType>::SizeType& i) const
                {
                    return i < this->storage.getCapacity() ? i : i - this->storage.getCapacity();
                }

            private:
                typename ArrayAccessPolicy<ElementType, StorageType>::SizeType  size{0};
                typename ArrayAccessPolicy<ElementType, StorageType>::IndexType head{0};
        };

        template
        <
            typename ElementType,
            template <typename> class StorageType = HeapStorage,
            template <typename, template <typename> class> class BufferAccessPolicyType = RingBufferAccessPolicy
        >
        class RingBuffer : public ArrayBuffer<ElementType, StorageType, BufferAccessPolicyType>
        {
            public:
                inline explicit RingBuffer(const typename StorageType<ElementType>::CapacityType& c)
                : ArrayBuffer<ElementType, StorageType, BufferAccessPolicyType>{c} {}

                inline auto getCapacity() const
                { 
                    return StorageType<ElementType>::getCapacity();
                }
        };
    }
}
