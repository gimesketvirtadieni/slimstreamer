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

#include <algorithm>
#include <memory>
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
    template<typename, typename> class BufferTemplate = HeapBuffer
>
class BufferPool
{
    public:
        using BufferType        = BufferTemplate<ElementType, std::unique_ptr<ElementType[]>>;
        using SizeType          = typename BufferType::SizeType;
        using PooledStorageType = std::unique_ptr<ElementType[], std::function<void(ElementType*)>>;
        using PooledBufferType  = BufferTemplate<ElementType, PooledStorageType>;

    public:
        inline explicit BufferPool(const SizeType& poolSize, const SizeType& bufferSize)
        {
            for (SizeType i = 0; i < poolSize; i++)
            {
                buffers.push_back(BufferWrapper{BufferType{std::make_unique<ElementType[]>(bufferSize), bufferSize}, true});
            }
        }

        inline PooledBufferType allocate()
        {
            for (SizeType i = 0; i < buffers.size(); i++)
            {
                auto& bufferWrapper = buffers[i];

                if (bufferWrapper.free)
                {
                    // creating a 'proxy-storage' object, which will mark this buffer as free upon destruction
                    auto pooledBufferStorage = PooledStorageType{bufferWrapper.buffer.getData(), [this, index = i](auto* releasedBuffer) {
                        buffers[index].free = true;
                    }};

                    // marking this buffer as used
                    bufferWrapper.free = false;

                    return PooledBufferType{std::move(pooledBufferStorage), bufferWrapper.buffer.getSize()};
                }
            }

            // this point is reach if no available buffer was found
            return PooledBufferType{PooledStorageType{}, 0};
        }

        inline const auto getAvailableSize() const
        {
            // auto may not be used here as count_if returns signed value
            SizeType s = std::count_if(buffers.begin(), buffers.end(), [](const auto& bufferWrapper)
            {
                return bufferWrapper.free;
            });

            return s;
        }

        inline const auto getSize() const
        {
            return buffers.size();
        }

    protected:
        struct BufferWrapper
        {
            BufferType buffer;
            bool       free;
        };

    private:
        std::vector<BufferWrapper> buffers;
};

}
}
}
