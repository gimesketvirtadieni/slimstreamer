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
class PointerWrapper
{
    public:
        inline explicit PointerWrapper(const std::size_t& s)
        : data{s > 0 ? std::make_unique<ElementType[]>(s) : std::unique_ptr<ElementType[]>()} {}

        inline auto* get() const
        {
            return data.get();
        }

    private:
        std::unique_ptr<ElementType[]> data;
};

template
<
    typename ElementType,
    typename StorageType = PointerWrapper<ElementType>
>
class DefaultHeapBufferViewPolicy
{
    public:
        using SizeType = std::size_t;

        inline explicit DefaultHeapBufferViewPolicy(StorageType d, const SizeType& s)
        : data{std::move(d)}
        , size{s} {}

        inline explicit DefaultHeapBufferViewPolicy(const SizeType& s)
        : DefaultHeapBufferViewPolicy{std::move(StorageType{s}), s} {}

        inline auto* getData() const
        {
            return data.get();
        }

        inline const auto getSize() const
        {
            return size;
        }

    private:
        StorageType data;
        SizeType    size;
};

template
<
    typename ElementType,
    typename StorageType = PointerWrapper<ElementType>,
    typename BufferViewPolicyType = DefaultHeapBufferViewPolicy<ElementType, StorageType>
>
class HeapBuffer : public BufferViewPolicyType
{
    public:
        inline explicit HeapBuffer(StorageType d, const typename BufferViewPolicyType::SizeType& s)
        : BufferViewPolicyType{std::move(d), s} {}

        inline explicit HeapBuffer(const typename BufferViewPolicyType::SizeType& s)
        : HeapBuffer{std::move(StorageType{s}), s} {}
};

}
}
}
