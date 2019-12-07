#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

#include "slim/util/buffer/HeapBuffer.hpp"


struct HeapBufferTestContext
{
	template
	<
		typename ElementType
	>
	using HeapBufferTest = slim::util::buffer::HeapBuffer<ElementType>;

	template<typename HeapBufferType>
	static void validateState(const HeapBufferType& buffer, const std::vector<int>& samples)
	{
		EXPECT_EQ(buffer.getSize(), samples.size());

		for (auto i{0u}; i < samples.size(); i++)
		{
			EXPECT_EQ(buffer.getData()[i], samples[i]);
		}
	}
};
