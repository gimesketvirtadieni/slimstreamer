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

#include <type_traits>

#include "slim/util/buffer/RingTest.hpp"


TEST_P(RingTestFixture, Constructor1)
{
	std::size_t capacity = GetParam();

	RingTest<int> ring{capacity};

	validateState(ring, capacity, {});
}

TEST_P(RingTestFixture, Constructor2)
{
	std::size_t capacity = GetParam();
	std::vector<int> samples;
	RingTest<int> ring1{capacity};

	for (int i = 0; i < GetParam(); i++)
	{
		ring1.push(i);
		samples.push_back(i);
	}
	RingTest<int> ring2 = std::move(ring1);

	validateState(ring2, capacity, samples);
}

TEST(RingTest, Constructor3)
{
	EXPECT_FALSE(std::is_trivially_copyable<RingTestFixture::RingTest<int>>::value);
}

TEST_P(RingTestFixture, Clear1)
{
	std::size_t capacity = GetParam();
	RingTest<int> ring{capacity};

	ring.push(1);
	ring.clear();

	validateState(ring, capacity, {});
}

TEST_P(RingTestFixture, Pop1)
{
	std::size_t capacity = GetParam();
	RingTest<int> ring{capacity};

	ring.pop();

	validateState(ring, capacity, {});
}

TEST_P(RingTestFixture, Pop2)
{
	std::size_t capacity = GetParam();
	RingTest<int> ring{capacity};

	ring.push(1);
	ring.pop();

	validateState(ring, capacity, {});
}

TEST_P(RingTestFixture, Pop3)
{
	std::size_t capacity = GetParam();
	RingTest<int> ring{capacity};

	if (capacity >= 2)
	{
		ring.push(1);
		ring.push(2);
		ring.pop();

		validateState(ring, capacity, {2});
	}
}

TEST(RingTest, Push1)
{
	std::size_t capacity = 1;
	RingTestFixture::RingTest<int> ring{capacity};

	ring.push(1);

	RingTestFixture::validateState(ring, capacity, {1});
}

TEST(RingTest, Push2)
{
	std::size_t capacity = 2;
	RingTestFixture::RingTest<int> ring{capacity};

	ring.push(1);
	ring.push(2);
	ring.push(3);

	RingTestFixture::validateState(ring, capacity, {2, 3});
}

TEST(RingTest, Access1)
{
	std::size_t capacity = 2;
	RingTestFixture::RingTest<int> ring{capacity};

	// making sure it is OK to access beyond data size but within capacity
	ring.push(1);
	ring[1];
}

INSTANTIATE_TEST_SUITE_P(RingInstantiation, RingTestFixture, testing::Values(0, 1, 2, 3));
