
#include <cstddef>  // std::size_t
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>

#include "slim/util/buffer/RingBuffer.hpp"


// keeping counters outside of the errors policies so that methods can be const
unsigned int onOffsetOutOfBoundCounter{0};
unsigned int onIndexOutOfRangeCounter{0};

struct BufferErrorsPolicyTest
{
	template<typename BufferType>
	void onIndexOutOfRange(BufferType& buffer, const typename BufferType::IndexType& i) const
	{
		onIndexOutOfRangeCounter++;
	}
};

struct StorageErrorsPolicyTest
{
	template<class StorageType>
	void onOffsetOutOfBound(StorageType& storage, const typename StorageType::OffsetType& offset) const
	{
		onOffsetOutOfBoundCounter++;
	}
};

template<typename ElementType>
using DefaultStorageTest = slim::util::HeapStorage<ElementType, StorageErrorsPolicyTest>;

template
<
	typename ElementType,
	class BufferErrorsPolicyType = BufferErrorsPolicyTest,
	template <typename> class StorageType = DefaultStorageTest,
	template <typename, class, template <typename> class> class BufferAccessPolicyType = slim::util::RingBufferAccessPolicy
>
using RingBufferTest = slim::util::RingBuffer<ElementType, BufferErrorsPolicyType, StorageType, BufferAccessPolicyType>;
