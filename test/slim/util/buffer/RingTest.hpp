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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "slim/util/buffer/Ring.hpp"


struct RingTestFixture : public ::testing::TestWithParam<std::size_t>
{
	template
	<
		typename ElementType,
		template <typename> class StorageType = slim::util::buffer::HeapBuffer,
		template <typename, template <typename> class> class RingViewPolicyType = slim::util::buffer::RingViewPolicy
	>
	using RingTest = slim::util::buffer::Ring<ElementType, StorageType, RingViewPolicyType>;

	template<typename RingType>
	void validateState(RingType& ring, const std::size_t& capacity, const std::vector<int>& samples)
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
