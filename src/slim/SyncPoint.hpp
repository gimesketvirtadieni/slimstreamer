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
#include "slim/util/Duration.hpp"
#include "slim/util/Timestamp.hpp"


namespace slim
{
	class SyncPoint
	{
		public:
			SyncPoint()
			: SyncPoint{util::Timestamp::now(), util::Duration{0}} {}

			SyncPoint(const util::Timestamp& timestamp, const util::BigInteger& frames, unsigned int samplingRate)
			: SyncPoint{timestamp, framesToDuration(frames, samplingRate)} {}

			SyncPoint(const util::Timestamp& t, const util::Duration& e)
			: timestamp{t}
			, timeElapsed{e} {}

			~SyncPoint() = default;
			SyncPoint(const SyncPoint& rhs) = default;
			SyncPoint& operator=(const SyncPoint& rhs) = default;
			SyncPoint(SyncPoint&& rhs) = default;
			SyncPoint& operator=(SyncPoint&& rhs) = default;

			inline auto calculateDrift(const SyncPoint& syncPoint) const
			{
				auto t1{timestamp - timeElapsed};
				auto t2{syncPoint.getTimestamp() - syncPoint.getTimeElapsed()};

				return t2 - t1;
			}

			inline static util::Duration framesToDuration(const util::BigInteger& frames, unsigned int samplingRate)
			{
				return util::Duration{(frames * 1000 / samplingRate) * 1000};
			}

			inline util::Timestamp getTimestamp() const
			{
				return timestamp;
			}

			inline util::Duration getTimeElapsed() const
			{
				return timeElapsed;
			}

		private:
			util::Timestamp timestamp;
			util::Duration  timeElapsed{0};
		};
}
