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

#include "slim/util/buffer/Storage.hpp"


namespace slim
{
	namespace util
	{
        template
        <
            typename ElementType,
            template <typename> class StorageType
        >
        class ArrayAccessPolicy
        {
            public:
                using SizeType  = typename StorageType<ElementType>::CapacityType;
                using IndexType = typename StorageType<ElementType>::OffsetType;

                inline explicit ArrayAccessPolicy(StorageType<ElementType>& s)
                : storage{s} {}

                inline auto getSize() const
                {
                    return storage.getCapacity();
                }

                inline typename StorageType<ElementType>::ReferenceType operator[](const typename ArrayAccessPolicy<ElementType, StorageType>::IndexType& i)
                {
                    return storage.getElement(mapToDataIndex(i));
                }

                inline typename StorageType<ElementType>::ConstReferenceType operator[](const typename ArrayAccessPolicy<ElementType, StorageType>::IndexType& i) const
                {
                    return storage.getElement(mapToDataIndex(i));
                }

            protected:
                inline auto mapToDataIndex(const IndexType& i) const
                {
                    return i;
                }

            protected:
                StorageType<ElementType>& storage;
        };

        template
        <
            typename ElementType,
            template <typename> class StorageType = HeapStorage,
            template <typename, template <typename> class> class BufferAccessPolicyType = ArrayAccessPolicy
        >
        class ArrayBuffer : protected StorageType<ElementType>, public BufferAccessPolicyType<ElementType, StorageType>
        {
            public:
                inline explicit ArrayBuffer(const typename StorageType<ElementType>::CapacityType& c)
                : StorageType<ElementType>{c}
                , BufferAccessPolicyType<ElementType, StorageType>{(StorageType<ElementType>&)*this} {}
        };
    }
}
