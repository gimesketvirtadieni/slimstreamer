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
        class RawBufferAccessPolicy
        {
            public:
                inline explicit RawBufferAccessPolicy(StorageType<ElementType>& s)
                : storage{s} {}

                inline auto getSize() const
                {
                    return storage.getCapacity();
                }

            protected:
                StorageType<ElementType>& storage;
        };

        template
        <
            typename ElementType = std::uint8_t,
            template <typename> class StorageType = ContinuousHeapStorage,
            template <typename, template <typename> class> class BufferAccessPolicyType = RawBufferAccessPolicy
        >
        class RawBuffer : protected StorageType<ElementType>, public BufferAccessPolicyType<ElementType, StorageType>
        {
            public:
                inline explicit RawBuffer(const typename StorageType<ElementType>::CapacityType& c)
                : StorageType<ElementType>{c}
                , BufferAccessPolicyType<ElementType, StorageType>{(StorageType<ElementType>&)*this} {}

                inline auto getCapacity() const
                {
                    return StorageType<ElementType>::getCapacity();
                }

                inline auto getBuffer() const
                {
                    return StorageType<ElementType>::getBuffer();
                }
        };
    }
}
