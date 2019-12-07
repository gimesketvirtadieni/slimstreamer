#include <vector>

#include "slim/util/buffer/RingTest.hpp"


TEST(Ring, Constructor1)
{
	std::size_t capacity{0};

	RingTestContext::RingTest<int> ring{capacity};

	RingTestContext::validateState(ring, capacity, {});
}

TEST(Ring, Clear1)
{
	std::size_t capacity{1};
	RingTestContext::RingTest<int> ring{capacity};

	ring.push(1);
	ring.clear();

	RingTestContext::validateState(ring, capacity, {});
}

TEST(Ring, Pop1)
{
	std::size_t capacity{1};
	RingTestContext::RingTest<int> ring{capacity};

	ring.pop();

	RingTestContext::validateState(ring, capacity, {});
}

TEST(Ring, Pop2)
{
	std::size_t capacity{1};
	RingTestContext::RingTest<int> ring{capacity};

	ring.push(1);
	ring.pop();

	RingTestContext::validateState(ring, capacity, {});
}

TEST(Ring, Pop3)
{
	std::size_t capacity{2};
	RingTestContext::RingTest<int> ring{capacity};

	ring.push(1);
	ring.push(2);
	ring.pop();

	RingTestContext::validateState(ring, capacity, {2});
}

TEST(Ring, Push1)
{
	std::size_t capacity{1};
	RingTestContext::RingTest<int> ring{capacity};

	ring.push(1);

	RingTestContext::validateState(ring, capacity, {1});
}

TEST(Ring, Push2)
{
	std::size_t capacity{2};
	RingTestContext::RingTest<int> ring{capacity};

	ring.push(1);

	RingTestContext::validateState(ring, capacity, {1});
}

TEST(Ring, Push3)
{
	std::size_t capacity{2};
	RingTestContext::RingTest<int> ring{capacity};

	ring.push(1);
	ring.push(2);
	ring.push(3);

	RingTestContext::validateState(ring, capacity, {2, 3});
}

TEST(Ring, Access1)
{
	std::size_t capacity{2};
	RingTestContext::RingTest<int> ring{capacity};

	ring.push(1);

	// making sure it is OK to access beyond data size but within capacity
	ring[1];
}
