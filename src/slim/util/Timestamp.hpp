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


namespace slim
{
	namespace util
	{
		class Timestamp
		{
			public:
				Timestamp()
				: timestamp{std::chrono::high_resolution_clock::now()} {}

				Timestamp(const std::chrono::high_resolution_clock::time_point& t)
				: timestamp{t} {}

				~Timestamp() = default;
				Timestamp(const Timestamp&) = default;
				Timestamp& operator=(const Timestamp&) = default;
				Timestamp(Timestamp&&) = default;
				Timestamp& operator=(Timestamp&&) = default;

				inline unsigned long long getMicroSeconds() const
				{
					return std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count();
				}

				inline unsigned long long getMilliSeconds() const
				{
					return std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count();
				}

				inline static Timestamp now()
				{
					return Timestamp{};
				}

				template<class _Rep, class _Period>
				inline Timestamp& operator+=(const std::chrono::duration<_Rep, _Period>& duration)
				{
					timestamp += duration;
					return *this;
				}

				template<class _Rep, class _Period>
				inline Timestamp& operator-=(const std::chrono::duration<_Rep, _Period>& duration)
				{
					timestamp -= duration;
					return *this;
				}

			private:
				std::chrono::high_resolution_clock::time_point timestamp;
		};

		template<class _Rep, class _Period>
		inline Timestamp operator+(Timestamp lhs, const std::chrono::duration<_Rep, _Period>& duration)
		{
			lhs += duration;
			return lhs;
		}

		template<class _Rep, class _Period>
		inline Timestamp operator-(Timestamp lhs, const std::chrono::duration<_Rep, _Period>& duration)
		{
			lhs -= duration;
			return lhs;
		}
	}
}
