#include <vector>

#include "slim/util/buffer/RingTest.hpp"

unsigned int RingTestContext::onIndexOutOfRangeCounter;

TEST(Ring, Constructor1)
{
	RingTestContext::onIndexOutOfRangeCounter = 0;
	std::size_t capacity{3};

	RingTestContext::RingTest<int> ring{capacity};

	RingTestContext::validateState(ring, capacity, {});
}

TEST(Ring, PopFront1)
{
	RingTestContext::onIndexOutOfRangeCounter = 0;
	std::size_t capacity{3};
	RingTestContext::RingTest<int> ring{capacity};

	ring.addBack(1);
	auto result{ring.shrinkFront()};

	EXPECT_TRUE(result);
	RingTestContext::validateState(ring, capacity, {});
}

TEST(Ring, PopFront2)
{
	RingTestContext::onIndexOutOfRangeCounter = 0;
	std::size_t capacity{1};
	RingTestContext::RingTest<int> ring{capacity};

	auto result{ring.shrinkFront()};

	EXPECT_FALSE(result);
	RingTestContext::validateState(ring, capacity, {});
}

TEST(Ring, PushBack1)
{
	RingTestContext::onIndexOutOfRangeCounter = 0;
	std::vector<int> samples;
	samples.push_back(1);
	RingTestContext::RingTest<int> ring{samples.size()};

	auto result{ring.addBack(samples[0])};

	EXPECT_EQ(0, result);
	RingTestContext::validateState(ring, samples.size(), samples);
}

TEST(Ring, PushBack2)
{
	RingTestContext::onIndexOutOfRangeCounter = 0;
	auto counter{0ul};
	std::vector<int> samples;

	// preparing sample data
	for (auto i{0u}; i < 10; i++)
	{
		samples.push_back(++counter);
	}

	// storing sample data in a ring buffer and validating its content
	RingTestContext::RingTest<int> ring{samples.size()};
	for (auto i{0u}; i < ring.getCapacity(); i++)
	{
		EXPECT_EQ(i, ring.addBack(samples[i]));
	}
	RingTestContext::validateState(ring, samples.size(), samples);

	// validating ring buffer 'shifts' as expected
	EXPECT_EQ(samples.size() - 1, ring.addBack(++counter));
	EXPECT_EQ(counter, ring[samples.size() - 1]);
}
