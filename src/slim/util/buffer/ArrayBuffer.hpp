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

#include <type_traits>

#include "slim/util/buffer/Storage.hpp"


namespace slim
{
	namespace util
	{
        class IgnoreArrayErrorsPolicy
        {
            public:
                template<typename BufferType>
                void onIndexOutOfRange(BufferType& buffer, const typename BufferType::IndexType& i) const {}
        };

        template<typename ElementType>
        using DefaultStorage = HeapStorage<ElementType, IgnoreStorageErrorsPolicy>;

        template
        <
            typename ElementType,
            class BufferErrorsPolicyType = IgnoreArrayErrorsPolicy,
            template <typename> class StorageType = DefaultStorage
        >
        class ArrayBufferAccessPolicy : public BufferErrorsPolicyType
        {
            public:
                using SizeType  = typename StorageType<ElementType>::CapacityType;
                using IndexType = typename StorageType<ElementType>::OffsetType;

                inline explicit ArrayBufferAccessPolicy(StorageType<ElementType>& s)
                : storage{s} {}

                inline auto& operator[](const typename ArrayBufferAccessPolicy<ElementType, BufferErrorsPolicyType, StorageType>::IndexType& i)
                {
                    return *(const_cast<ElementType*>(static_cast<const ArrayBufferAccessPolicy<ElementType, BufferErrorsPolicyType, StorageType>&>(*this).getElementByIndex(i, isIndexOutOfRange(i, getSize()))));
                }

                inline auto& operator[](const typename ArrayBufferAccessPolicy<ElementType, BufferErrorsPolicyType, StorageType>::IndexType& i) const
                {
                    return *this->getElementByIndex(i, isIndexOutOfRange(i, getSize()));
                }

                inline auto getSize() const
                {
                    return storage.getCapacity();
                }

            protected:
                inline auto* getElementByIndex(const IndexType& i, bool indexOutOfRange) const
                {
                    if (!indexOutOfRange)
                    {
                        return storage.getElement(i);
                    }
                    else
                    {
                        BufferErrorsPolicyType::onIndexOutOfRange(*this, i);
                        return (ElementType*)nullptr;
                    }
                }

                inline auto isIndexOutOfRange(const IndexType& i, const SizeType& size) const
                {
                    auto result{false};

                    if constexpr(std::is_signed<IndexType>::value)
                    {
                        if (i < 0)
                        {
                            result = true;
                        }
                    }

                    if (!result)
                    {
                        result = (i >= size);
                    }

                    return result;
                }

                StorageType<ElementType>& storage;
        };

        template
        <
            typename ElementType,
            class BufferErrorsPolicyType = IgnoreArrayErrorsPolicy,
            template <typename> class StorageType = DefaultStorage,
            template <typename, class, template <typename> class> class BufferAccessPolicyType = ArrayBufferAccessPolicy
        >
        class ArrayBuffer : protected StorageType<ElementType>, public BufferAccessPolicyType<ElementType, BufferErrorsPolicyType, StorageType>
        {
            public:
                inline explicit ArrayBuffer(const typename StorageType<ElementType>::CapacityType& c)
                : StorageType<ElementType>{c}
                , BufferAccessPolicyType<ElementType, BufferErrorsPolicyType, StorageType>{(StorageType<ElementType>&)*this} {}
        };
    }
}
