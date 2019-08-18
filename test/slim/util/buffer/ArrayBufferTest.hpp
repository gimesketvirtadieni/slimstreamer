#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

#include "slim/util/buffer/ArrayBuffer.hpp"


struct ArrayBufferTestContext
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
	using ArrayBufferAccessPolicyType = slim::util::buffer::ArrayBufferAccessPolicy<ElementType, BufferErrorsPolicyTest, StorageType>;

	template
	<
		typename ElementType,
		template <typename> class StorageType = DefaultStorageTest,
		template <typename, template <typename> class> class BufferAccessPolicyType = ArrayBufferAccessPolicyType
	>
	class ArrayBufferTest : public slim::util::buffer::ArrayBuffer<ElementType, StorageType, BufferAccessPolicyType>
	{
		public:
			inline explicit ArrayBufferTest(const typename StorageType<ElementType>::CapacityType& c)
			: slim::util::buffer::ArrayBuffer<ElementType, StorageType, BufferAccessPolicyType>{c} {}

			// overwriting visibility to make it public
			using BufferAccessPolicyType<ElementType, StorageType>::getElementByIndex;
			using BufferAccessPolicyType<ElementType, StorageType>::isIndexOutOfRange;
	};

	template<typename ArrayBufferType>
	static void validateState(ArrayBufferType& arrayBuffer, const std::vector<int>& samples)
	{
		EXPECT_EQ(arrayBuffer.getSize(), samples.size());

		for (auto i{std::size_t{0}}; i < samples.size(); i++)
		{
			EXPECT_EQ(arrayBuffer[i], samples[i]);
		}

		EXPECT_EQ(ArrayBufferTestContext::onIndexOutOfRangeCounter, 0);
		EXPECT_EQ(ArrayBufferTestContext::onOffsetOutOfBoundCounter, 0);
	}
};
