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
#include <memory>   // std::unique_ptr
#include <utility>  // std::as_const

namespace slim
{
	namespace util
	{
        class IgnoreStorageErrorsPolicy
        {
            public:
                template<class StorageType>
                void onOffsetOutOfBound(StorageType& storage, const typename StorageType::OffsetType& offset) const {}
        };

        template
        <
            typename ElementType,
            class ErrorPolicyType
        >
        class HeapStorage : public ErrorPolicyType
        {
            public:
                using CapacityType = std::size_t;
                using OffsetType   = std::size_t;

				HeapStorage() = default;
				~HeapStorage() = default;
				HeapStorage(const HeapStorage&) = delete;             // non-copyable
				HeapStorage& operator=(const HeapStorage&) = delete;  // non-assignable
				HeapStorage(HeapStorage&&) = default;
				HeapStorage& operator=(HeapStorage&&) = default;

                inline explicit HeapStorage(const CapacityType& c)
                : capacity{c}
                , bufferPtr{std::make_unique<ElementType[]>(capacity)} {}

                inline auto getCapacity() const
                {
                    return capacity;
                }

                inline auto* getElement(const OffsetType& offset)
                {
                    return this->getElementByOffset(offset);
                }

                inline auto* getElement(const OffsetType& offset) const
                {
                    return const_cast<HeapStorage<ElementType, ErrorPolicyType>*>(this)->getElementByOffset(offset);
                }

            protected:
                inline auto* getElementByOffset(const OffsetType& offset) const
                {
                    if (offset < capacity)
                    {
                        return (bufferPtr.get() + offset);
                    }
                    else
                    {
                        ErrorPolicyType::onOffsetOutOfBound(*this, offset);
                        return (ElementType*)nullptr;
                    }
                }

                inline auto* getBuffer() const
                {
                    return bufferPtr.get();
                }

            private:
                CapacityType                   capacity;
                std::unique_ptr<ElementType[]> bufferPtr;
        };

        template
        <
            typename ElementType,
            class ErrorPolicyType
        >
        class ContinuousHeapStorage : public HeapStorage<ElementType, ErrorPolicyType>
        {
            public:
                inline explicit ContinuousHeapStorage(const typename HeapStorage<ElementType, ErrorPolicyType>::CapacityType& s)
                : HeapStorage<ElementType, ErrorPolicyType>{s} {}

                inline auto getBuffer() const
                {
                    return HeapStorage<ElementType, ErrorPolicyType>::getBuffer();
                }
        };
    }
}
