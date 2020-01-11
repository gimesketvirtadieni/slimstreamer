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
class DefaultHeapBufferStorage
{
    public:
        using PointerType = std::unique_ptr<ElementType[]>;
        using SizeType    = std::size_t;

        inline explicit DefaultHeapBufferStorage(const SizeType& s)
        : data{new ElementType[s]}
        , size{s} {}

        PointerType data;
        SizeType    size;
};

template
<
    typename ElementType,
    template <typename> class StorageType = DefaultHeapBufferStorage
>
class DefaultHeapBufferViewPolicy
{
    public:
        using SizeType = typename StorageType<ElementType>::SizeType;

        inline explicit DefaultHeapBufferViewPolicy(StorageType<ElementType> d)
        : storage{std::move(d)} {}

        inline explicit DefaultHeapBufferViewPolicy(const SizeType& s)
        : storage{s} {}

        inline auto* getData() const
        {
            return storage.data.get();
        }

        inline const auto getSize() const
        {
            return storage.size;
        }

    private:
        StorageType<ElementType> storage;
};

template
<
    typename ElementType,
    template <typename> class StorageType = DefaultHeapBufferStorage,
    template <typename, template <typename> class> class BufferViewPolicyType = DefaultHeapBufferViewPolicy
>
class HeapBuffer : public BufferViewPolicyType<ElementType, StorageType>
{
    public:
        inline explicit HeapBuffer(StorageType<ElementType> d)
        : BufferViewPolicyType<ElementType, StorageType>{std::move(d)} {}

        inline explicit HeapBuffer(const typename BufferViewPolicyType<ElementType, StorageType>::SizeType& s = 0)
        : BufferViewPolicyType<ElementType, StorageType>{s} {}
};

}
}
}
