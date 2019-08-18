#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "slim/util/buffer/Ring.hpp"


struct RingTestContext
{
	// keeping counters outside of the errors policies so that methods can be const
	static unsigned int onIndexOutOfRangeCounter;

	template<typename ElementType>
	using DefaultStorageTest = slim::util::buffer::HeapStorage<ElementType>;

	struct BufferErrorsPolicyTest
	{
		template<typename BufferType>
		void onIndexOutOfRange(BufferType& buffer, const typename BufferType::IndexType& i) const
		{
			onIndexOutOfRangeCounter++;
		}
	};

	template
	<
		typename ElementType,
		template <typename> class StorageType = DefaultStorageTest
	>
	using RingAccessPolicyType = slim::util::buffer::RingAccessPolicy<ElementType, BufferErrorsPolicyTest, StorageType>;

	template
	<
		typename ElementType,
		template <typename> class StorageType = DefaultStorageTest,
		template <typename, template <typename> class> class BufferAccessPolicyType = RingAccessPolicyType
	>
	using RingTest = slim::util::buffer::Ring<ElementType, StorageType, BufferAccessPolicyType>;

	template<typename RingType>
	static void validateState(RingType& ring, const std::size_t& capacity, const std::vector<int>& samples)
	{
		EXPECT_EQ(ring.getCapacity(), capacity);
		EXPECT_EQ(ring.getSize(), samples.size());

		if (0 < samples.size())
		{
			EXPECT_FALSE(ring.isEmpty());

			for (auto i{std::size_t{0}}; i < samples.size(); i++)
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

		EXPECT_EQ(RingTestContext::onIndexOutOfRangeCounter, 0);
	}
};
