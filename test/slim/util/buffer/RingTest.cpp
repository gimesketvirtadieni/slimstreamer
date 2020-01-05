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

#include "slim/util/buffer/RingTest.hpp"


TEST_P(RingTestFixture, Constructor1)
{
	std::size_t capacity = GetParam();

	RingTest<int> ring{capacity};

	validateState(ring, capacity, {});
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

TEST_P(RingTestFixture, Push1)
{
	std::size_t capacity = GetParam();
	RingTest<int> ring{capacity};

	if (capacity > 0)
	{
		ring.push(1);

		validateState(ring, capacity, {1});
	}
}

TEST_P(RingTestFixture, Push2)
{
	std::size_t capacity = GetParam();
	RingTest<int> ring{capacity};

	if (capacity == 2)
	{
		ring.push(1);
		ring.push(2);
		ring.push(3);

		validateState(ring, capacity, {2, 3});
	}
}

TEST_P(RingTestFixture, Access1)
{
	std::size_t capacity = GetParam();
	RingTest<int> ring{capacity};

	// making sure it is OK to access beyond data size but within capacity
	if (capacity >= 2)
	{
		ring.push(1);
		ring[1];
	}
}

INSTANTIATE_TEST_SUITE_P(RingInstantiation, RingTestFixture, testing::Values(0, 1, 2, 3));
