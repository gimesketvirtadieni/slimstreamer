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
		using DurationType = std::chrono::duration<int64_t, std::micro>;

		public:
			SyncPoint()
			: SyncPoint{util::Timestamp::now(), DurationType{0}} {}

			SyncPoint(const util::Timestamp& timestamp, const util::BigInteger& frames, unsigned int samplingRate)
			: SyncPoint{timestamp, framesToDuration(frames, samplingRate)} {}

			SyncPoint(const util::Timestamp& t, const DurationType& e)
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

				return std::chrono::duration_cast<DurationType>(t2 - t1);
			}

			inline static DurationType framesToDuration(const util::BigInteger& frames, unsigned int samplingRate)
			{
				return DurationType{(frames * 1000 / samplingRate) * 1000};
			}

			inline util::Timestamp getTimestamp() const
			{
				return timestamp;
			}

			inline DurationType getTimeElapsed() const
			{
				return timeElapsed;
			}

		private:
			util::Timestamp timestamp;
			DurationType    timeElapsed{0};
		};
}
