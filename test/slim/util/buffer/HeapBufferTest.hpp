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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

#include "slim/util/buffer/HeapBuffer.hpp"


struct HeapBufferTestFixture : public ::testing::TestWithParam<std::size_t>
{
	template
	<
		typename ElementType
	>
	using HeapBufferTest = slim::util::buffer::HeapBuffer<ElementType>;

	template<typename HeapBufferType>
	void validateState(const HeapBufferType& buffer, const std::vector<int>& samples)
	{
		EXPECT_EQ(buffer.getSize(), samples.size());

		for (auto i{0u}; i < samples.size(); i++)
		{
			EXPECT_EQ(buffer.getData()[i], samples[i]);
		}
	}
};
