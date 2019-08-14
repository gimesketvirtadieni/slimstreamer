#include <vector>

#include "slim/util/buffer/RingBufferTest.hpp"


unsigned int RingBufferTestContext::onOffsetOutOfBoundCounter;
unsigned int RingBufferTestContext::onIndexOutOfRangeCounter;


TEST(RingBuffer, Constructor1)
{
	RingBufferTestContext::onIndexOutOfRangeCounter = 0;
	RingBufferTestContext::onOffsetOutOfBoundCounter = 0;
	std::size_t capacity{3};

	RingBufferTestContext::RingBufferTest<int> ringBuffer{capacity};

	RingBufferTestContext::validateState(ringBuffer, capacity, {});
}

TEST(RingBuffer, PopFront1)
{
	RingBufferTestContext::onIndexOutOfRangeCounter = 0;
	RingBufferTestContext::onOffsetOutOfBoundCounter = 0;
	std::size_t capacity{3};
	RingBufferTestContext::RingBufferTest<int> ringBuffer{capacity};

	ringBuffer.addBack(1);
	auto result{ringBuffer.shrinkFront()};

	EXPECT_TRUE(result);
	RingBufferTestContext::validateState(ringBuffer, capacity, {});
}

TEST(RingBuffer, PopFront2)
{
	RingBufferTestContext::onIndexOutOfRangeCounter = 0;
	RingBufferTestContext::onOffsetOutOfBoundCounter = 0;
	std::size_t capacity{1};
	RingBufferTestContext::RingBufferTest<int> ringBuffer{capacity};

	auto result{ringBuffer.shrinkFront()};

	EXPECT_FALSE(result);
	RingBufferTestContext::validateState(ringBuffer, capacity, {});
}

TEST(RingBuffer, PushBack1)
{
	RingBufferTestContext::onIndexOutOfRangeCounter = 0;
	RingBufferTestContext::onOffsetOutOfBoundCounter = 0;
	std::vector<int> samples;
	samples.push_back(1);
	RingBufferTestContext::RingBufferTest<int> ringBuffer{samples.size()};

	auto result{ringBuffer.addBack(samples[0])};

	EXPECT_EQ(0, result);
	RingBufferTestContext::validateState(ringBuffer, samples.size(), samples);
}

TEST(RingBuffer, PushBack2)
{
	RingBufferTestContext::onIndexOutOfRangeCounter = 0;
	RingBufferTestContext::onOffsetOutOfBoundCounter = 0;
	auto counter{0ul};
	std::vector<int> samples;

	// preparing sample data
	for (auto i{0u}; i < 10; i++)
	{
		samples.push_back(++counter);
	}

	// storing sample data in a ring buffer and validating its content
	RingBufferTestContext::RingBufferTest<int> ringBuffer{samples.size()};
	for (auto i{0u}; i < ringBuffer.getCapacity(); i++)
	{
		EXPECT_EQ(i, ringBuffer.addBack(samples[i]));
	}
	RingBufferTestContext::validateState(ringBuffer, samples.size(), samples);

	// validating ring buffer 'shifts' as expected
	EXPECT_EQ(samples.size() - 1, ringBuffer.addBack(++counter));
	EXPECT_EQ(counter, ringBuffer[samples.size() - 1]);
}
