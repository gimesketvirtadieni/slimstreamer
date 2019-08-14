#include "slim/util/buffer/ArrayBufferTest.hpp"


unsigned int ArrayBufferTestContext::onOffsetOutOfBoundCounter;
unsigned int ArrayBufferTestContext::onIndexOutOfRangeCounter;


TEST(ArrayBuffer, Constructor1)
{
	ArrayBufferTestContext::onIndexOutOfRangeCounter = 0;
	ArrayBufferTestContext::onOffsetOutOfBoundCounter = 0;
	std::size_t capacity{0};

	ArrayBufferTestContext::ArrayBufferTest<int> arrayBuffer{capacity};

	ArrayBufferTestContext::validateState(arrayBuffer, {});
}

// TODO: this should be part of ArrayBufferTest
/*
TEST(RingBuffer, IndexOutOfRange)
{
	auto t1{onIndexOutOfRangeCounter};
	auto t2{onOffsetOutOfBoundCounter};
	
	RingBufferTest<int> ringBuffer{1};
	EXPECT_EQ(0, ringBuffer.addBack(1));

	// accessing the second element which is out of range
	ringBuffer[1];

	EXPECT_EQ(onIndexOutOfRangeCounter, t1 + 1);
	EXPECT_EQ(onOffsetOutOfBoundCounter, t2);
}
*/
