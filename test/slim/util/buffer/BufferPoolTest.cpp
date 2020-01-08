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

#include "slim/util/buffer/BufferPoolTest.hpp"


TEST_P(BufferPoolTestFixture, Constructor1)
{
    std::size_t poolSize = GetParam();
    std::size_t bufferSize = 1;
    BufferPoolTest<int> bufferPool{poolSize, bufferSize};

    EXPECT_EQ(bufferPool.getSize(), poolSize);
    EXPECT_EQ(bufferPool.getAvailableSize(), poolSize);
}

TEST(BufferPoolTest, Constructor2)
{
    std::size_t poolSize = 2;
    std::size_t bufferSize = 1;
    BufferPoolTestFixture::BufferPoolTest<int> bufferPool1{poolSize, bufferSize};
    BufferPoolTestFixture::BufferPoolTest<int> bufferPool2{0, 0};

    {
        auto allocatedBuffer = bufferPool1.allocate();
        allocatedBuffer.getData()[0] = 11;
        bufferPool2 = std::move(bufferPool1);

        EXPECT_EQ(bufferPool1.getAvailableSize(), 0);
        EXPECT_EQ(allocatedBuffer.getData()[0], 11);
    }
    {
        auto allocatedBuffer = bufferPool2.allocate();

        EXPECT_EQ(bufferPool1.getAvailableSize(), 0);
        EXPECT_EQ(allocatedBuffer.getData()[0], 11);
    }
}

TEST_P(BufferPoolTestFixture, Constructor3)
{
	EXPECT_FALSE(std::is_trivially_copyable<BufferPoolTest<int>>::value);
}

TEST_P(BufferPoolTestFixture, Allocate1)
{
    std::size_t poolSize = GetParam();
    std::size_t bufferSize = 2;
    BufferPoolTest<int> bufferPool{poolSize, bufferSize};
    std::vector<BufferPoolTest<int>::PooledBufferType> allocatedBuffers;

    // repeating this part 3 times
    for (auto i = 0u; i < 3; i++)
    {
        allocatedBuffers.clear();

        // exhausting pool
        EXPECT_EQ(bufferPool.getAvailableSize(), poolSize);
        for (auto j = 0u; j < poolSize; j++)
        {
            auto allocatedBuffer = bufferPool.allocate();
            EXPECT_NE(allocatedBuffer.getData(), nullptr);
            EXPECT_EQ(allocatedBuffer.getSize(), bufferSize);

            allocatedBuffers.push_back(std::move(allocatedBuffer));
        }
        EXPECT_EQ(bufferPool.getAvailableSize(), 0);

        auto allocatedBuffer = bufferPool.allocate();
        EXPECT_EQ(allocatedBuffer.getData(), nullptr);
    }
}


TEST(BufferPoolTest, Allocate2)
{
    std::size_t poolSize = 1;
    std::size_t bufferSize = 2;
    BufferPoolTestFixture::BufferPoolTest<int> bufferPool{poolSize, bufferSize};
    std::vector<BufferPoolTestFixture::BufferPoolTest<int>::PooledBufferType> allocatedBuffers;

    {
        auto allocatedBuffer = bufferPool.allocate();
        allocatedBuffer.getData()[0] = 11;
        allocatedBuffer.getData()[1] = 22;
    }
    {
        auto allocatedBuffer = bufferPool.allocate();
        EXPECT_EQ(allocatedBuffer.getData()[0], 11);
        EXPECT_EQ(allocatedBuffer.getData()[1], 22);
    }
}

INSTANTIATE_TEST_SUITE_P(BufferPoolInstantiation, BufferPoolTestFixture, testing::Values(0, 1, 2, 3));
