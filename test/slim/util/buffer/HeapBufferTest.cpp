#include "slim/util/buffer/HeapBufferTest.hpp"


TEST(HeapBuffer, Constructor1)
{
	std::size_t size{0};

	HeapBufferTestContext::HeapBufferTest<int> buffer{size};

	HeapBufferTestContext::validateState(buffer, {});
}

TEST(HeapBuffer, getElement1)
{
	std::vector<int> samples{11, 22};
	HeapBufferTestContext::HeapBufferTest<int> buffer{samples.size()};
	
	for (auto i{0u}; i < samples.size(); i++)
	{
		buffer.getData()[i] = samples[i];
	}

	HeapBufferTestContext::validateState(buffer, samples);
}
