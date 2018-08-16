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

#include <cstdint>  // std::u..._t types
#include <unordered_map>

#include "slim/util/Timestamp.hpp"


namespace slim
{
	namespace util
	{
		class TimestampCache
		{
			public:
				TimestampCache() = default;
			   ~TimestampCache() = default;
				TimestampCache(const TimestampCache&) = delete;             // non-copyable
				TimestampCache& operator=(const TimestampCache&) = delete;  // non-assignable
				TimestampCache(TimestampCache&&) = default;
				TimestampCache& operator=(TimestampCache&&) = default;

				inline auto create(const Timestamp& timestamp = Timestamp{})
				{
					timestampMap.emplace(++counter, timestamp);

					return counter;
				}

				inline void erase(std::uint32_t key)
				{
					timestampMap.erase(key);
				}

				inline auto find(std::uint32_t key)
				{
					std::optional<Timestamp> result{std::nullopt};

					auto found{timestampMap.find(key)};
					if (found != timestampMap.end())
					{
						result = found->second;
					}

					return result;
				}

				inline auto size()
				{
					return timestampMap.size();
				}

				inline void update(std::uint32_t key, const Timestamp& timestamp)
				{
					// throws std::out_of_range if key does not exist
					timestampMap.at(key) = timestamp;
				}

			private:
				std::unordered_map<std::uint32_t, Timestamp> timestampMap;
				std::uint32_t                                counter{0};
		};
	}
}
