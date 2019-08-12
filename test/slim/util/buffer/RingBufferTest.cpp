#include <vector>

#include "slim/util/buffer/RingBufferTest.hpp"


template<typename RingBufferType>
void validateState(RingBufferType& ringBuffer, const std::size_t& capacity, const std::vector<int>& samples, unsigned int t1, unsigned int t2)
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

	EXPECT_EQ(onIndexOutOfRangeCounter, t1);
	EXPECT_EQ(onOffsetOutOfBoundCounter, t2);
}

TEST(RingBuffer, Constructor1)
{
	auto t1{onIndexOutOfRangeCounter};
	auto t2{onOffsetOutOfBoundCounter};
	std::size_t capacity{3};

	RingBufferTest<int> ringBuffer{capacity};

	validateState(ringBuffer, capacity, {}, t1, t2);
}

TEST(RingBuffer, PopFront1)
{
	auto t1{onIndexOutOfRangeCounter};
	auto t2{onOffsetOutOfBoundCounter};
	std::size_t capacity{3};
	RingBufferTest<int> ringBuffer{capacity};

	ringBuffer.addBack(1);
	auto result{ringBuffer.shrinkFront()};

	EXPECT_TRUE(result);
	validateState(ringBuffer, capacity, {}, t1, t2);
}

TEST(RingBuffer, PopFront2)
{
	auto t1{onIndexOutOfRangeCounter};
	auto t2{onOffsetOutOfBoundCounter};
	std::size_t capacity{1};
	RingBufferTest<int> ringBuffer{capacity};

	auto result{ringBuffer.shrinkFront()};

	EXPECT_FALSE(result);
	validateState(ringBuffer, capacity, {}, t1, t2);
}

TEST(RingBuffer, PushBack1)
{
	auto t1{onIndexOutOfRangeCounter};
	auto t2{onOffsetOutOfBoundCounter};
	std::vector<int> samples;
	samples.push_back(1);
	RingBufferTest<int> ringBuffer{samples.size()};

	auto result{ringBuffer.addBack(samples[0])};

	EXPECT_EQ(0, result);
	validateState(ringBuffer, samples.size(), samples, t1, t2);
}

TEST(RingBuffer, PushBack2)
{
	auto t1{onIndexOutOfRangeCounter};
	auto t2{onOffsetOutOfBoundCounter};
	auto counter{0ul};
	std::vector<int> samples;

	// preparing sample data
	for (auto i{0u}; i < 10; i++)
	{
		samples.push_back(++counter);
	}

	// storing sample data in a ring buffer and validating its content
	RingBufferTest<int> ringBuffer{samples.size()};
	for (auto i{0u}; i < ringBuffer.getCapacity(); i++)
	{
		EXPECT_EQ(i, ringBuffer.addBack(samples[i]));
	}
	validateState(ringBuffer, samples.size(), samples, t1, t2);

	// validating ring buffer 'shifts' as expected
	EXPECT_EQ(samples.size() - 1, ringBuffer.addBack(++counter));
	EXPECT_EQ(counter, ringBuffer[samples.size() - 1]);
}
