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
        namespace buffer
        {
            template
            <
                typename ElementType,
                template <typename> class StorageType = HeapStorage
            >
            class RawBufferAccessPolicy
            {
                public:
                    inline explicit RawBufferAccessPolicy(StorageType<ElementType>& s)
                    : storage{s} {}

                protected:
                    StorageType<ElementType>& storage;
            };

            template
            <
                typename ElementType,
                template <typename> class StorageType = ContinuousHeapStorage,
                template <typename, template <typename> class> class BufferAccessPolicyType = RawBufferAccessPolicy
            >
            class RawBuffer : public StorageType<ElementType>, public BufferAccessPolicyType<ElementType, StorageType>
            {
                public:
                    inline explicit RawBuffer(const typename StorageType<ElementType>::CapacityType& c)
                    : StorageType<ElementType>{c}
                    , BufferAccessPolicyType<ElementType, StorageType>{(StorageType<ElementType>&)*this} {}

                    // overwriting visibility to make it public
                    using StorageType<ElementType>::getCapacity;
                    using StorageType<ElementType>::getBuffer;
            };
        }
    }
}
