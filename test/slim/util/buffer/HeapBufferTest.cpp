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

#include "slim/util/buffer/HeapBufferTest.hpp"

TEST_P(HeapBufferTestFixture, Constructor1)
{
	std::size_t size = GetParam();
	HeapBufferTest<int> buffer{size};

    EXPECT_EQ(buffer.getSize(), size);
	if (size == 0)
	{
		EXPECT_EQ(buffer.getData(), nullptr);
	}
}

TEST_P(HeapBufferTestFixture, Constructor2)
{
	std::size_t size = GetParam();
	std::vector<int> samples;
	HeapBufferTest<int> buffer1{size};

	for (int i = 0; i < GetParam(); i++)
	{
		buffer1.getData()[i] = i;
		samples.push_back(i);
	}
	HeapBufferTest<int> buffer2 = std::move(buffer1);

	validateState(buffer2, samples);
}

TEST_P(HeapBufferTestFixture, Constructor3)
{
	EXPECT_FALSE(std::is_trivially_copyable<HeapBufferTest<int>>::value);
}

TEST_P(HeapBufferTestFixture, getElement1)
{
	std::vector<int> samples(GetParam());
	HeapBufferTest<int> buffer{samples.size()};

	for (auto i{0u}; i < samples.size(); i++)
	{
		samples[i] = i * 11;
		buffer.getData()[i] = samples[i];
	}

	validateState(buffer, samples);
}

TEST_P(HeapBufferTestFixture, getElement2)
{
	std::vector<int> samples(GetParam());
	slim::util::buffer::DefaultHeapBufferStorage<int> storage{samples.size()};

	// saving some data in a separatelly created storage
	for (auto i{0u}; i < samples.size(); i++)
	{
		storage.data.get()[i] = samples[i];
	}
	HeapBufferTest<int> buffer{std::move(storage)};

	validateState(buffer, samples);
}

INSTANTIATE_TEST_SUITE_P(HeapBufferInstantiation, HeapBufferTestFixture, testing::Values(0, 1, 2, 3));
