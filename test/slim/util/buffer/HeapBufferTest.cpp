#include "slim/util/buffer/HeapBufferTest.hpp"


TEST(HeapBuffer, Constructor1)
{
	std::size_t size{0};

	HeapBufferTestContext::HeapBufferTest<int> buffer{size};

	HeapBufferTestContext::validateState(buffer, {});
	EXPECT_EQ(buffer.getData(), nullptr);
}

TEST(HeapBuffer, Constructor2)
{
	std::size_t size{11};
	slim::util::buffer::DefaultHeapBufferStorage<int> data{size};
	HeapBufferTestContext::HeapBufferTest<int> buffer{std::move(data)};

	EXPECT_EQ(buffer.getSize(), size);
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

TEST(HeapBuffer, getElement2)
{
	std::vector<int> samples{11, 22};
	slim::util::buffer::DefaultHeapBufferStorage<int> storage{samples.size()};
	for (auto i{0u}; i < samples.size(); i++)
	{
		storage.data.get()[i] = samples[i];
	}
	HeapBufferTestContext::HeapBufferTest<int> buffer{std::move(storage)};

	HeapBufferTestContext::validateState(buffer, samples);
}
