#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "slim/util/buffer/RingBuffer.hpp"


struct RingBufferTestContext
{
	// keeping counters outside of the errors policies so that methods can be const
	static unsigned int onOffsetOutOfBoundCounter;
	static unsigned int onIndexOutOfRangeCounter;

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
	using DefaultStorageTest = slim::util::HeapStorage<ElementType, RingBufferTestContext::StorageErrorsPolicyTest>;

	template
	<
		typename ElementType,
		class BufferErrorsPolicyType = RingBufferTestContext::BufferErrorsPolicyTest,
		template <typename> class StorageType = DefaultStorageTest,
		template <typename, class, template <typename> class> class BufferAccessPolicyType = slim::util::RingBufferAccessPolicy
	>
	using RingBufferTest = slim::util::RingBuffer<ElementType, BufferErrorsPolicyType, StorageType, BufferAccessPolicyType>;

	template<typename RingBufferType>
	static void validateState(RingBufferType& ringBuffer, const std::size_t& capacity, const std::vector<int>& samples)
	{
		EXPECT_EQ(ringBuffer.getCapacity(), capacity);
		EXPECT_EQ(ringBuffer.getSize(), samples.size());

		if (0 < samples.size())
		{
			EXPECT_FALSE(ringBuffer.isEmpty());

			for (auto i{std::size_t{0}}; i < samples.size(); i++)
			{
				EXPECT_EQ(ringBuffer[i], samples[i]);
			}
		}
		else
		{
			EXPECT_TRUE(ringBuffer.isEmpty());
		}

		if (samples.size() < capacity)
		{
			EXPECT_FALSE(ringBuffer.isFull());
		}
		else
		{
			EXPECT_TRUE(ringBuffer.isFull());
		}

		EXPECT_EQ(RingBufferTestContext::onIndexOutOfRangeCounter, 0);
		EXPECT_EQ(RingBufferTestContext::onOffsetOutOfBoundCounter, 0);
	}
};
