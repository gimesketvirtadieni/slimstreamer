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

#include <utility>  // std::as_const

#include "slim/util/buffer/Array.hpp"


namespace slim
{
	namespace util
	{
        namespace buffer
        {
            template
            <
                typename ElementType,
                class BufferErrorsPolicyType = IgnoreArrayErrorsPolicy,
                template <typename> class StorageType = DefaultStorage
            >
            class RingAccessPolicy : protected ArrayAccessPolicy<ElementType, BufferErrorsPolicyType, StorageType>
            {
                public:
                    using SizeType  = typename ArrayAccessPolicy<ElementType, BufferErrorsPolicyType, StorageType>::SizeType;
                    using IndexType = typename ArrayAccessPolicy<ElementType, BufferErrorsPolicyType, StorageType>::IndexType;

                    inline explicit RingAccessPolicy(StorageType<ElementType>& s)
                    : ArrayAccessPolicy<ElementType, BufferErrorsPolicyType, StorageType>{s} {}

                    inline auto& operator[](const IndexType& i) const
                    {
                        return *this->getElementByIndex(getAbsoluteIndex(i));
                    }

                    inline auto& operator[](const IndexType& i)
                    {
                        return const_cast<ElementType&>(*std::as_const(*this).getElementByIndex(getAbsoluteIndex(i)));
                    }

                    inline void clear()
                    {
                        size = 0;
                    }

                    inline SizeType getSize() const
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

                    inline auto shrinkBack()
                    {
                        auto result{false};

                        if (0 < size)
                        {
                            size--;
                            result = true;
                        }

                        return result;
                    }

                    inline auto shrinkFront()
                    {
                        auto result{false};

                        if (0 < size)
                        {
                            size--;
                            head   = normalizeIndex(++head);
                            result = true;
                        }

                        return result;
                    }

                    inline auto addFront(const ElementType& item)
                    {
                        head = (0 < head ? head : *this->storage.getCapacity()) - 1;
                        if (!isFull())
                        {
                            size++;
                        }
                        *this->storage.getElement(head) = item;

                        return IndexType{size - 1};
                    }

                    inline auto addBack(const ElementType& item)
                    {
                        if (!isFull())
                        {
                            size++;
                        }
                        else
                        {
                            head = normalizeIndex(head + 1);
                        }
                        *(this->storage.getBuffer() + normalizeIndex(head + size - 1)) = item;

                        return IndexType{size - 1};
                    }

                protected:
                    inline auto getAbsoluteIndex(const IndexType& i) const
                    {
                        return i < this->storage.getCapacity() ? normalizeIndex(head + i) : i;
                    }

                    inline auto normalizeIndex(const IndexType& i) const
                    {
                        return i >= this->storage.getCapacity() ? i - this->storage.getCapacity() : i;
                    }

                private:
                    IndexType head{0};
                    SizeType  size{0};
            };

            template
            <
                typename ElementType,
                template <typename> class StorageType = DefaultStorage
            >
            using DefaultRingAccessPolicyType = RingAccessPolicy<ElementType, IgnoreArrayErrorsPolicy, StorageType>;

            template
            <
                typename ElementType,
                template <typename> class StorageType = DefaultStorage,
                template <typename, template <typename> class> class BufferAccessPolicyType = DefaultRingAccessPolicyType
            >
            class Ring : public Array<ElementType, StorageType, BufferAccessPolicyType>
            {
                public:
                    inline explicit Ring(const typename StorageType<ElementType>::CapacityType& c)
                    : Array<ElementType, StorageType, BufferAccessPolicyType>{c} {}

                    inline auto getCapacity() const
                    {
                        return StorageType<ElementType>::getCapacity();
                    }
            };
        }
    }
}
