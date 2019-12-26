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
#include <vector>

#include "slim/util/buffer/HeapBuffer.hpp"


namespace slim
{
namespace util
{
namespace buffer
{

template
<
    typename ElementType,
    template <typename> class StorageType = HeapBuffer
>
class DefaultArrayViewPolicy
{
    public:
        using SizeType  = typename StorageType<ElementType>::SizeType;
        using IndexType = std::size_t;

        inline explicit DefaultArrayViewPolicy(const SizeType& s)
        : storage{s} {}

        inline const auto& operator[](const IndexType& i) const
        {
            return storage.getData()[i];
        }

        inline auto& operator[](const IndexType& i)
        {
            // getData may be 'non-const' thus it may not be delegated to const version of this method
            return storage.getData()[i];
        }

        inline const auto getSize() const
        {
            return storage.getSize();
        }

    private:
        StorageType<ElementType> storage;
};

template
<
    typename ElementType,
    template <typename> class StorageType = HeapBuffer,
    template <typename, template <typename> class> class ArrayViewPolicyType = DefaultArrayViewPolicy
>
class Array : public ArrayViewPolicyType<ElementType, StorageType>
{
    public:
        inline explicit Array(const typename ArrayViewPolicyType<ElementType, StorageType>::SizeType& s)
        : ArrayViewPolicyType<ElementType, StorageType>{s} {}
};

}
}
}
