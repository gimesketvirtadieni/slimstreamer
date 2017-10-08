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


/*
 * This code is based on https://rnestler.github.io/c-list-of-scopeguard.html
 */
namespace slim
{
	class ScopeGuardBase
	{
		public:
			ScopeGuardBase() noexcept
			: active{true} {}


			ScopeGuardBase(ScopeGuardBase&& rhs) noexcept
			: active{rhs.active}
			{
				rhs.commit();
			}

			ScopeGuardBase& operator=(ScopeGuardBase&& rhs) noexcept
			{
				active = rhs.active;
				rhs.commit();
				return *this;
			}

			ScopeGuardBase(const ScopeGuardBase&) = delete;             // non-copyable
			ScopeGuardBase& operator=(const ScopeGuardBase&) = delete;  // non-assignable

			inline void commit() noexcept
			{
				active = false;
			}

			inline bool isActive() noexcept
			{
				return active;
			}

		protected:
			~ScopeGuardBase() = default;

		private:
			bool active;
	};
}
