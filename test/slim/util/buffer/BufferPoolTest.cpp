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

#include "slim/util/buffer/BufferPoolTest.hpp"


TEST_P(BufferPoolTestFixture, Constructor1)
{
    std::size_t poolSize = GetParam();
    std::size_t bufferSize = 1;
    BufferPoolTest<int> bufferPool{poolSize, bufferSize};

    EXPECT_EQ(bufferPool.getSize(), poolSize);
    EXPECT_EQ(bufferPool.getAvailableSize(), poolSize);
}


TEST_P(BufferPoolTestFixture, Allocate1)
{
    std::size_t poolSize = GetParam();
    std::size_t bufferSize = 2;
    BufferPoolTest<int> bufferPool{poolSize, bufferSize};
    std::vector<BufferPoolTest<int>::PooledBufferType> allocatedBuffers;

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
