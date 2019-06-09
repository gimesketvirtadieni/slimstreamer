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
            class ErrorPolicyType = IgnoreArrayErrorsPolicy,
            template <typename, class> class StorageType = HeapStorage
        >
        class RingBufferAccessPolicy : public ArrayAccessPolicy<ElementType, ErrorPolicyType, StorageType>
        {
            public:
                using SizeType  = typename ArrayAccessPolicy<ElementType, ErrorPolicyType, StorageType>::SizeType;
                using IndexType = typename ArrayAccessPolicy<ElementType, ErrorPolicyType, StorageType>::IndexType;

                inline explicit RingBufferAccessPolicy(StorageType<ElementType, IgnoreStorageErrorsPolicy>& s)
                : ArrayAccessPolicy<ElementType, ErrorPolicyType, StorageType>{s} {}

                inline auto& operator[](const IndexType& i)
                {
                    return *(const_cast<ElementType*>(static_cast<const RingBufferAccessPolicy<ElementType, ErrorPolicyType, StorageType>&>(*this).getElementByIndex(normalizeIndex(firstOccupied + i), getSize())));
                }

                inline auto& operator[](const IndexType& i) const
                {
                    return *this->getElementByIndex(normalizeIndex(firstOccupied + i), getSize());
                }

                inline void clear()
                {
                    empty = true;
                    firstOccupied = firstVacant = 0;
                }

                inline SizeType getSize() const
                {
                    if (!empty)
                    {
                        LOG(DEBUG) << LABELS{"proto"} << "HERE 21"
                            << " getSize="       << (firstOccupied < firstVacant ? 0 : this->storage.getCapacity()) + firstVacant - firstOccupied
                            << " firstVacant="   << firstVacant
                            << " firstOccupied=" << firstOccupied;

                        // adding capacity is needed to make sure that index of first vacant element is always greater than index first occupied element
                        return (firstOccupied < firstVacant ? 0 : this->storage.getCapacity()) + firstVacant - firstOccupied;
                    }
                    else
                    {
                        return 0;
                    }
                }

                inline auto isEmpty() const
                {
                    return empty;
                }

                inline auto isFull() const
                {
                    return !empty && firstOccupied == firstVacant;
                }

                inline auto popBack()
                {
                    auto result{false};

                    if (!empty)
                    {
                        firstVacant = normalizeIndex(firstVacant - 1);
                        empty       = (firstOccupied == firstVacant);
                        result      = true;
                    }

                    return result;
                }

                inline auto popFront()
                {
                    auto result{false};

                    if (!empty)
                    {
                        firstOccupied = normalizeIndex(firstOccupied + 1);
                        empty         = (firstOccupied == firstVacant);
                        result        = true;
                    }

                    return result;
                }

                inline auto pushFront(const ElementType& item)
                {
                    auto result{false};
                    auto f{normalizeIndex(firstOccupied - 1)};

                    if (!empty)
                    {
                        if (f == firstVacant)
                        {
                            result      = true;
                            firstVacant = normalizeIndex(firstVacant - 1);
                        }
                    }
                    else
                    {
                        empty = false;
                    }

                    *this->storage.getElement(firstOccupied) = item;
                    firstOccupied                            = f;

                    return result;
                }

                inline auto pushBack(const ElementType& item)
                {
                    auto result{false};
                    auto f{normalizeIndex(firstVacant + 1)};

                    if (!empty)
                    {
                        // if wrap around
                        if (f == firstOccupied)
                        {
                            result        = true;
                            firstOccupied = normalizeIndex(firstOccupied + 1);
                        }
                    }
                    else
                    {
                        empty = false;
                    }

                    *this->storage.getElement(firstVacant) = item;
                    firstVacant                            = f;

                    return result;
                }

            protected:
                inline auto normalizeIndex(const typename ArrayAccessPolicy<ElementType, ErrorPolicyType, StorageType>::SizeType& i) const
                {
                    return i < this->storage.getCapacity() ? i : i - this->storage.getCapacity();
                }

            private:
                bool      empty{true};
                IndexType firstOccupied{0};
                IndexType firstVacant{0};
        };

        template
        <
            typename ElementType,
            class ErrorPolicyType = IgnoreArrayErrorsPolicy,
            template <typename, class> class StorageType = HeapStorage,
            template <typename, class, template <typename, class> class> class BufferAccessPolicyType = RingBufferAccessPolicy
        >
        class RingBuffer : public ArrayBuffer<ElementType, ErrorPolicyType, StorageType, BufferAccessPolicyType>
        {
            public:
                inline explicit RingBuffer(const typename StorageType<ElementType, IgnoreStorageErrorsPolicy>::CapacityType& c)
                : ArrayBuffer<ElementType, ErrorPolicyType, StorageType, BufferAccessPolicyType>{c} {}

                inline auto getCapacity() const
                { 
                    return StorageType<ElementType, IgnoreStorageErrorsPolicy>::getCapacity();
                }
        };
    }
}
