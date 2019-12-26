#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

#include "slim/util/buffer/Array.hpp"


struct ArrayTestContext
{
	template
	<
		typename ElementType
	>
	class VectorStorage
	{
		public:
			using SizeType = std::size_t;

			inline explicit VectorStorage(const std::size_t& s)
			: storage(s) {}

			inline auto& getData()
			{
				return storage;
			}

			inline const auto getSize() const
			{
				return storage.size();
			}

		private:
			std::vector<ElementType> storage;
	};

	template
	<
		typename ElementType,
		template <typename> class StorageType = slim::util::buffer::HeapBuffer,
		template <typename, template <typename> class> class ArrayViewPolicyType = slim::util::buffer::DefaultArrayViewPolicy
	>
	using ArrayTest = slim::util::buffer::Array<ElementType, StorageType, ArrayViewPolicyType>;

	template<typename ArrayType>
	static void validateState(ArrayType& array, const std::vector<int>& samples)
	{
		EXPECT_EQ(array.getSize(), samples.size());

		for (auto i{0u}; i < samples.size(); i++)
		{
			EXPECT_EQ(array[i], samples[i]);
		}
	}
};
