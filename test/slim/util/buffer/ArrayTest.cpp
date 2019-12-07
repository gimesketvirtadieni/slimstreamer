#include "slim/util/buffer/ArrayTest.hpp"


TEST(Array, Constructor1)
{
	std::size_t size{0};

	ArrayTestContext::ArrayTest<int> array{size};

	ArrayTestContext::validateState(array, {});
}

TEST(Array, getElement1)
{
	std::vector<int> samples{11, 22};
	ArrayTestContext::ArrayTest<int> array{samples.size()};

	for (auto i{0u}; i < samples.size(); i++)
	{
		array[i] = samples[i];
	}

	ArrayTestContext::validateState(array, samples);
}
