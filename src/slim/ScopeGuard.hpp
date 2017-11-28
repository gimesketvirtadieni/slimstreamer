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

#include <optional>
#include <utility>

#include "ScopeGuardBase.hpp"


/*
 * This C++17 code is based on https://rnestler.github.io/c-list-of-scopeguard.html
 */
namespace slim
{
	template<class Fun>
	class ScopeGuard : public ScopeGuardBase
	{
		public:
			ScopeGuard(Fun f) noexcept
			: ScopeGuardBase{}
			, fun{std::move(f)} {}

			~ScopeGuard() noexcept
			{
				// fun in the condition is required as move constructor creates an empty fun
				if (isActive() && fun) try
				{
					fun.value()();
				}
				catch(...) {}
			}

			ScopeGuard(const ScopeGuard&) = delete;            // non-copyable
			ScopeGuard& operator=(const ScopeGuard&) = delete; // non-assignable

			ScopeGuard(ScopeGuard&& rhs) noexcept
			: fun{std::move(rhs.fun)}
			{
				rhs.commit();
			}

			ScopeGuard& operator=(ScopeGuard&& rhs) noexcept
			{
				using std::swap;

				rhs.commit();

				swap(fun, rhs.fun);

				return *this;
			}

		private:
			std::optional<Fun> fun;
	};


	template<class Fun>
	ScopeGuard<Fun> makeScopeGuard(Fun f) noexcept
	{
		return ScopeGuard<Fun>(std::move(f));
	}
}
