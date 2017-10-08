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

#include "ScopeGuardBase.hpp"


/*
 * This code is based on https://rnestler.github.io/c-list-of-scopeguard.html
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
				if (isActive())
				{
					try
					{
						fun();
					}
					catch(...) {}
				}
			}

			ScopeGuard(ScopeGuard&& rhs) noexcept
			: ScopeGuardBase{std::move(rhs)}
			, fun{std::move(rhs.fun)} {}

			ScopeGuard& operator=(ScopeGuard&& rhs) noexcept
			{
				operator =(rhs);
				return *this;
			}

			ScopeGuard() = delete;
			ScopeGuard(const ScopeGuard&) = delete;            // non-copyable
			ScopeGuard& operator=(const ScopeGuard&) = delete; // non-assignable

		private:
			Fun fun;
	};


	template<class Fun>
	ScopeGuard<Fun> makeScopeGuard(Fun f) noexcept
	{
		return ScopeGuard<Fun>(std::move(f));
	}
}
