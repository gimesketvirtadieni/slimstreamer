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

TEST(ArrayBuffer, getElementByIndex1)
{
	ArrayBufferTestContext::ArrayBufferTest<int> arrayBuffer{1};
	auto value{11u};
	arrayBuffer[0] = value;

	EXPECT_EQ(*arrayBuffer.getElementByIndex(0), value);
	EXPECT_EQ(ArrayBufferTestContext::onIndexOutOfRangeCounter, 0);
	EXPECT_EQ(ArrayBufferTestContext::onOffsetOutOfBoundCounter, 0);
}

TEST(ArrayBuffer, getElementByIndex2)
{
	ArrayBufferTestContext::ArrayBufferTest<int> arrayBuffer{1};

	EXPECT_EQ(arrayBuffer.getElementByIndex(1), nullptr);
	EXPECT_EQ(ArrayBufferTestContext::onIndexOutOfRangeCounter, 1);
	EXPECT_EQ(ArrayBufferTestContext::onOffsetOutOfBoundCounter, 0);
}
