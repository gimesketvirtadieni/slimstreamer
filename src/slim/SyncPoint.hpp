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

#include <chrono>

#include "slim/util/BigInteger.hpp"
#include "slim/util/Timestamp.hpp"


namespace slim
{
	class SyncPoint
	{
		public:
			SyncPoint()
			: SyncPoint{util::Timestamp::now(), 0} {}

			SyncPoint(const util::Timestamp& t, const util::BigInteger& f)
			: timestamp{t}
			, frames{f} {}

			~SyncPoint() = default;
			SyncPoint(const SyncPoint& rhs) = default;
			SyncPoint& operator=(const SyncPoint& rhs) = default;
			SyncPoint(SyncPoint&& rhs) = default;
			SyncPoint& operator=(SyncPoint&& rhs) = default;

			inline auto calculateDrift(const SyncPoint& syncPoint, unsigned int samplingRate)
			{
				// TODO: work in progress
				return std::chrono::microseconds{0};
			}

			inline auto getTimestamp()
			{
				return timestamp;
			}

			inline auto getFrames() const
			{
				return frames;
			}

		private:
			util::Timestamp  timestamp;
			util::BigInteger frames{0};
		};
}
