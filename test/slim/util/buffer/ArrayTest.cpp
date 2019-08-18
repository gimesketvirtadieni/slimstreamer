#include "slim/util/buffer/ArrayTest.hpp"


unsigned int ArrayTestContext::onIndexOutOfRangeCounter;


TEST(Array, Constructor1)
{
	ArrayTestContext::onIndexOutOfRangeCounter = 0;
	std::size_t capacity{0};

	ArrayTestContext::ArrayTest<int> arrayBuffer{capacity};

	ArrayTestContext::validateState(arrayBuffer, {});
}

TEST(Array, getElementByIndex1)
{
	ArrayTestContext::ArrayTest<int> array{1};
	auto value{11u};
	array[0] = value;

	EXPECT_EQ(*array.getElementByIndex(0), value);
	EXPECT_EQ(ArrayTestContext::onIndexOutOfRangeCounter, 0);
}

TEST(Array, getElementByIndex2)
{
	ArrayTestContext::ArrayTest<int> array{1};

	EXPECT_EQ(array.getElementByIndex(1), nullptr);
	EXPECT_EQ(ArrayTestContext::onIndexOutOfRangeCounter, 1);
}
