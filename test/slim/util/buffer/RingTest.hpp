#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "slim/util/buffer/Ring.hpp"


struct RingTestContext
{
	template
	<
		typename ElementType,
		template <typename> class StorageType = slim::util::buffer::HeapBuffer,
		template <typename, template <typename> class> class BufferViewPolicyType = slim::util::buffer::RingViewPolicy
	>
	using RingTest = slim::util::buffer::Ring<ElementType, StorageType, BufferViewPolicyType>;

	template<typename RingType>
	static void validateState(RingType& ring, const std::size_t& capacity, const std::vector<int>& samples)
	{
		EXPECT_EQ(ring.getCapacity(), capacity);
		EXPECT_EQ(ring.getSize(), samples.size());

		if (0 < samples.size())
		{
			EXPECT_FALSE(ring.isEmpty());

			for (auto i{0u}; i < samples.size(); i++)
			{
				EXPECT_EQ(ring[i], samples[i]);
			}
		}
		else
		{
			EXPECT_TRUE(ring.isEmpty());
		}

		if (samples.size() < capacity)
		{
			EXPECT_FALSE(ring.isFull());
		}
		else
		{
			EXPECT_TRUE(ring.isFull());
		}
	}
};
