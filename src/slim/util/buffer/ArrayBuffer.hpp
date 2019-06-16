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

        template
        <
            typename ElementType,
            class ErrorPolicyType = IgnoreArrayErrorsPolicy,
            template <typename, class> class StorageType = HeapStorage
        >
        class ArrayAccessPolicy : public ErrorPolicyType
        {
            public:
                using SizeType  = typename StorageType<ElementType, IgnoreStorageErrorsPolicy>::CapacityType;
                using IndexType = typename StorageType<ElementType, IgnoreStorageErrorsPolicy>::OffsetType;

                inline explicit ArrayAccessPolicy(StorageType<ElementType, IgnoreStorageErrorsPolicy>& s)
                : storage{s} {}

                inline auto& operator[](const typename ArrayAccessPolicy<ElementType, ErrorPolicyType, StorageType>::IndexType& i)
                {
                    return *(const_cast<ElementType*>(static_cast<const ArrayAccessPolicy<ElementType, ErrorPolicyType, StorageType>&>(*this).getElementByIndex(i, isIndexOutOfRange(i, getSize()))));
                }

                inline auto& operator[](const typename ArrayAccessPolicy<ElementType, ErrorPolicyType, StorageType>::IndexType& i) const
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
                    if (indexOutOfRange)
                    {
                        ErrorPolicyType::onIndexOutOfRange(*this, i);
                    }

                    return storage.getElement(i);
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

                StorageType<ElementType, IgnoreStorageErrorsPolicy>& storage;
        };

        template
        <
            typename ElementType,
            class ErrorPolicyType = IgnoreArrayErrorsPolicy,
            template <typename, class> class StorageType = HeapStorage,
            template <typename, class, template <typename, class> class> class BufferAccessPolicyType = ArrayAccessPolicy
        >
        class ArrayBuffer : protected StorageType<ElementType, IgnoreStorageErrorsPolicy>, public BufferAccessPolicyType<ElementType, ErrorPolicyType, StorageType>
        {
            public:
                inline explicit ArrayBuffer(const typename StorageType<ElementType, IgnoreStorageErrorsPolicy>::CapacityType& c)
                : StorageType<ElementType, IgnoreStorageErrorsPolicy>{c}
                , BufferAccessPolicyType<ElementType, ErrorPolicyType, StorageType>{(StorageType<ElementType, IgnoreStorageErrorsPolicy>&)*this} {}
        };
    }
}
