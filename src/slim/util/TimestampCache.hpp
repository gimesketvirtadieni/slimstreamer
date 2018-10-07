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

#include "slim/util/Timestamp.hpp"


namespace slim
{
	namespace util
	{
		namespace ts = type_safe;

		template<std::uint32_t TotalElements>
		class TimestampCache
		{
			public:
				TimestampCache()
				{
					std::for_each(timestamps.begin(), timestamps.end(), [](auto& timestamp)
					{
						timestamp.reset();
					});
				}

			   ~TimestampCache() = default;
				TimestampCache(const TimestampCache&) = delete;             // non-copyable
				TimestampCache& operator=(const TimestampCache&) = delete;  // non-assignable
				TimestampCache(TimestampCache&&) = default;
				TimestampCache& operator=(TimestampCache&&) = default;

				inline std::uint32_t capacity()
				{
					return timestamps.size();
				}

				inline void clear()
				{
					std::for_each(timestamps.begin(), timestamps.end(), [](auto& timestamp)
					{
						timestamp.reset();
					});
				}

				inline auto load(std::uint32_t key)
				{
					ts::optional<Timestamp> result{ts::nullopt};
					auto                    index{key - 1};

					if (index < timestamps.size())
					{
						result = timestamps[index];
					}

					return result;
				}

				inline std::uint32_t size()
				{
					return std::count_if(timestamps.begin(), timestamps.end(), [](const auto& timestamp)
					{
						return timestamp.has_value();
					});
				}

				inline auto store(const Timestamp& timestamp = Timestamp{})
				{
					if (next >= timestamps.size())
					{
						next = 0;
					}

					auto index{next++};
					timestamps[index] = timestamp;

					return index + 1;
				}

				inline auto update(std::uint32_t key, const Timestamp& timestamp)
				{
					auto result{false};
					auto index{key - 1};

					if (index < timestamps.size() && timestamps[index].has_value())
					{
						timestamps[index] = timestamp;
						result            = true;
					}

					return result;
				}

			private:
				std::array<ts::optional<Timestamp>, TotalElements> timestamps;
				std::uint32_t                                      next{0};
		};
	}
}
