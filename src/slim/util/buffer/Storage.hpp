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


namespace slim
{
	namespace util
	{
        template <typename ElementType>
        class HeapStorage
        {
            public:
                using CapacityType       = std::size_t;
                using OffsetType         = std::size_t;
                using ReferenceType      = ElementType&;
                using ConstReferenceType = const ElementType&;
                using PointerType        = ElementType*;

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

                inline ReferenceType getElement(const OffsetType& offset)
                {
                    return bufferPtr.get()[offset];
                }

                inline ConstReferenceType getElement(const OffsetType& offset) const
                {
                    return bufferPtr.get()[offset];
                }

            protected:
                inline auto getBuffer() const
                {
                    return bufferPtr.get();
                }

            private:
                CapacityType                   capacity;
                std::unique_ptr<ElementType[]> bufferPtr;
        };

        template <typename ElementType>
        class ContinuousHeapStorage : public HeapStorage<ElementType>
        {
            public:
                inline explicit ContinuousHeapStorage(const typename HeapStorage<ElementType>::CapacityType& s)
                : HeapStorage<ElementType>{s} {}

                inline auto getBuffer() const
                {
                    return HeapStorage<ElementType>::getBuffer();
                }
        };
    }
}
