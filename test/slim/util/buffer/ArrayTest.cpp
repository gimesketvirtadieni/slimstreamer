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

#include "slim/util/buffer/ArrayTest.hpp"


TEST_P(ArrayTestFixture, Constructor1)
{
	std::size_t size = GetParam();
	ArrayTest<int> array{size};

    EXPECT_EQ(array.getSize(), size);
}

TEST_P(ArrayTestFixture, getElement1)
{
	std::vector<int> samples(GetParam());
	ArrayTest<int> array{samples.size()};

	for (auto i{0u}; i < samples.size(); i++)
	{
		samples[i] = i * 11;
		array[i]   = samples[i];
	}

	validateState(array, samples);
}

TEST_P(ArrayTestFixture, getElement2)
{
	std::vector<int> samples(GetParam());
	ArrayTest<int, VectorStorage> array{samples.size()};

	for (auto i{0u}; i < samples.size(); i++)
	{
		samples[i] = i * 11;
		array[i]   = samples[i];
	}

	validateState(array, samples);
}

INSTANTIATE_TEST_SUITE_P(ArrayInstantiation, ArrayTestFixture, testing::Values(0, 1, 2, 3));
