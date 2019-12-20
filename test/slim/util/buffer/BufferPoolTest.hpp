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

#include "slim/util/buffer/BufferPool.hpp"
#include "slim/util/buffer/HeapBuffer.hpp"


struct BufferPoolTestFixture : public ::testing::TestWithParam<std::size_t>
{
	template
    <
		typename ElementType
	>
	using BufferPoolTest = slim::util::buffer::BufferPool<ElementType, slim::util::buffer::HeapBuffer>;
};
