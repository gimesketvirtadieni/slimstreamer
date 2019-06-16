
#include <cstddef>  // std::size_t
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>

#include "slim/util/buffer/RingBuffer.hpp"


// keeping 
unsigned int onIndexOutOfRangeCounter{0};

struct RingBufferErrorsPolicyTest
{
	template<typename BufferType>
	void onIndexOutOfRange(BufferType& buffer, const typename BufferType::IndexType& i) const
	{
		onIndexOutOfRangeCounter++;
	}
};

template<typename RingBufferType>
void validateState(RingBufferType& ringBuffer, const std::size_t& capacity, const std::vector<int>& samples)
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

	auto t{onIndexOutOfRangeCounter + 1};
	ringBuffer[capacity];
	EXPECT_EQ(onIndexOutOfRangeCounter, t);
}

TEST(RingBuffer, Constructor1)
{
	std::size_t capacity{3};

	slim::util::RingBuffer<int, RingBufferErrorsPolicyTest> ringBuffer{capacity};

	validateState(ringBuffer, capacity, {});
}

TEST(RingBuffer, PopFront1)
{
	std::size_t capacity{3};
	slim::util::RingBuffer<int, RingBufferErrorsPolicyTest> ringBuffer{capacity};

	ringBuffer.pushBack(1);
	auto result{ringBuffer.popFront()};

	EXPECT_TRUE(result);
	validateState(ringBuffer, capacity, {});
}

TEST(RingBuffer, PopFront2)
{
	std::size_t capacity{1};
	slim::util::RingBuffer<int, RingBufferErrorsPolicyTest> ringBuffer{capacity};

	auto result{ringBuffer.popFront()};

	EXPECT_FALSE(result);
	validateState(ringBuffer, capacity, {});
}

TEST(RingBuffer, PushBack1)
{
	std::vector<int> samples;
	samples.push_back(1);
	slim::util::RingBuffer<int, RingBufferErrorsPolicyTest> ringBuffer{samples.size()};

	auto result{ringBuffer.pushBack(samples[0])};

	EXPECT_FALSE(result);
	validateState(ringBuffer, samples.size(), samples);
}

TEST(RingBuffer, PushBack2)
{
	long counter{0};
	std::vector<int> samples;

	for (unsigned s{0}; s < 100; s++)
	{
		samples.push_back(++counter);

		slim::util::RingBuffer<int, RingBufferErrorsPolicyTest> ringBuffer{samples.size()};
		for (unsigned j{0}; j <= ringBuffer.getCapacity(); j++)
		{
			for (unsigned i{0}; i < samples.size(); i++)
			{
				EXPECT_FALSE(ringBuffer.pushBack(samples[i]));
			}
			validateState(ringBuffer, samples.size(), samples);

			EXPECT_TRUE(ringBuffer.pushBack(++counter));
			EXPECT_EQ(ringBuffer[ringBuffer.getSize() - 1], counter);

			for (unsigned i{0}; i < samples.size(); i++)
			{
				EXPECT_TRUE(ringBuffer.popFront());
			}
			validateState(ringBuffer, samples.size(), std::vector<int>{});
		}
	}
}
