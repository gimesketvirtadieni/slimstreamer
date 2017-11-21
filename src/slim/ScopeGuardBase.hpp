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

#include <utility>


/*
 * This C++17 code is based on https://rnestler.github.io/c-list-of-scopeguard.html
 */
namespace slim
{
	class ScopeGuardBase
	{
		public:
			ScopeGuardBase() noexcept
			: active{true} {}

			// using Rule Of Zero
			ScopeGuardBase(const ScopeGuardBase&) = delete;             // non-copyable
			ScopeGuardBase& operator=(const ScopeGuardBase&) = delete;  // non-copyable
			ScopeGuardBase(ScopeGuardBase&& rhs) = default;
			ScopeGuardBase& operator=(ScopeGuardBase&& rhs) = default;

			inline void commit() noexcept
			{
				active = false;
			}

			inline bool isActive() noexcept
			{
				return active;
			}

		protected:
			// making this destructor protected to avoid using virtualization
			// it will not allow leaks like this:
			// ScopeGuardBase* p = new ScopeGuard<...>([]() {...});
			// delete p;  <- will cause a compilation error
			~ScopeGuardBase() = default;

		private:
			bool active;
	};
}
