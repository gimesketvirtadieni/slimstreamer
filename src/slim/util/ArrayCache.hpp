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

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>  // std::u..._t types
#include <type_safe/optional.hpp>


namespace slim
{
	namespace util
	{
		namespace ts = type_safe;

		template<typename ElementType, std::uint32_t TotalElements>
		class ArrayCache
		{
			public:
				ArrayCache()
				{
					std::for_each(elements.begin(), elements.end(), [](auto& element)
					{
						element.reset();
					});
				}

				~ArrayCache() = default;
				ArrayCache(const ArrayCache&) = delete;             // non-copyable
				ArrayCache& operator=(const ArrayCache&) = delete;  // non-assignable
				ArrayCache(ArrayCache&&) = default;
				ArrayCache& operator=(ArrayCache&&) = default;

				inline std::uint32_t capacity()
				{
					return elements.size();
				}

				inline void clear()
				{
					std::for_each(elements.begin(), elements.end(), [](auto& element)
					{
						element.reset();
					});
				}

				inline auto load(std::uint32_t key)
				{
					ts::optional<ElementType> result{ts::nullopt};
					auto                      index{key - 1};

					if (index < elements.size())
					{
						result = elements[index];
					}

					return result;
				}

				inline std::uint32_t size()
				{
					return std::count_if(elements.begin(), elements.end(), [](const auto& element)
					{
						return element.has_value();
					});
				}

				inline auto store(const ElementType& element)
				{
					if (next >= elements.size())
					{
						next = 0;
					}

					auto index{next++};
					elements[index] = element;

					return index + 1;
				}

				inline auto update(std::uint32_t key, const ElementType& element)
				{
					auto result{false};
					auto index{key - 1};

					if (index < elements.size() && elements[index].has_value())
					{
						elements[index] = element;
						result          = true;
					}

					return result;
				}

			private:
				std::array<ts::optional<Timestamp>, TotalElements> elements;
				std::uint32_t                                      next{0};
		};
	}
}
