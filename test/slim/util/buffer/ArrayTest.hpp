#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

#include "slim/util/buffer/Array.hpp"


struct ArrayTestContext
{
	// keeping counters outside of the errors policies so that methods can be const
	static unsigned int onOffsetOutOfBoundCounter;
	static unsigned int onIndexOutOfRangeCounter;

	struct StorageErrorsPolicyTest
	{
		template<class StorageType>
		void onOffsetOutOfBound(StorageType& storage, const typename StorageType::OffsetType& offset) const
		{
			onOffsetOutOfBoundCounter++;
		}
	};

	template<typename ElementType>
	using DefaultStorageTest = slim::util::buffer::HeapStorage<ElementType, StorageErrorsPolicyTest>;

	struct BufferErrorsPolicyTest
	{
		template<typename BufferType>
		void onIndexOutOfRange(BufferType& buffer, const typename BufferType::IndexType& i) const
		{
			onIndexOutOfRangeCounter++;
		}
	};

	template
	<
		typename ElementType,
		template <typename> class StorageType = DefaultStorageTest
	>
	using ArrayAccessPolicyType = slim::util::buffer::ArrayAccessPolicy<ElementType, BufferErrorsPolicyTest, StorageType>;

	template
	<
		typename ElementType,
		template <typename> class StorageType = DefaultStorageTest,
		template <typename, template <typename> class> class BufferAccessPolicyType = ArrayAccessPolicyType
	>
	class ArrayTest : public slim::util::buffer::Array<ElementType, StorageType, BufferAccessPolicyType>
	{
		public:
			inline explicit ArrayTest(const typename StorageType<ElementType>::CapacityType& c)
			: slim::util::buffer::Array<ElementType, StorageType, BufferAccessPolicyType>{c} {}

			// overwriting visibility to make it public
			using BufferAccessPolicyType<ElementType, StorageType>::getElementByIndex;
			using BufferAccessPolicyType<ElementType, StorageType>::isIndexOutOfRange;
	};

	template<typename ArrayType>
	static void validateState(ArrayType& arrayBuffer, const std::vector<int>& samples)
	{
		EXPECT_EQ(arrayBuffer.getSize(), samples.size());

		for (auto i{std::size_t{0}}; i < samples.size(); i++)
		{
			EXPECT_EQ(arrayBuffer[i], samples[i]);
		}

		EXPECT_EQ(ArrayTestContext::onIndexOutOfRangeCounter, 0);
		EXPECT_EQ(ArrayTestContext::onOffsetOutOfBoundCounter, 0);
	}
};
