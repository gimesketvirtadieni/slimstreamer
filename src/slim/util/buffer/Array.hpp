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
class DefaultArrayViewPolicy : protected StorageType<ElementType>
{
    public:
        using SizeType  = typename StorageType<ElementType>::SizeType;
        using IndexType = std::size_t;

        inline explicit DefaultArrayViewPolicy(const SizeType& s)
        : StorageType<ElementType>{s} {}

        inline const auto& operator[](const IndexType& i) const
        {
            return this->getData()[i];
        }

        inline auto& operator[](const IndexType& i)
        {
            return const_cast<ElementType&>(std::as_const(*this)[i]);
        }

        // overwriting visibility to make it public
        using StorageType<ElementType>::getSize;
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
