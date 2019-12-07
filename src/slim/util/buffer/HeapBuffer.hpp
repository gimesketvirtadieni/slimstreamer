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
namespace buffer
{

template
<
    typename ElementType
>
class HeapBuffer
{
    public:
        using SizeType = std::size_t;

        inline explicit HeapBuffer(const SizeType& s)
        : size{s}
        , dataPtr{std::make_unique<ElementType[]>(s)} {}

        inline auto* getData() const
        {
            return dataPtr.get();
        }

        inline auto getSize() const
        {
            return size;
        }

    private:
        SizeType                       size;
        std::unique_ptr<ElementType[]> dataPtr;
};

}
}
}
