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
				if (isActive())
				{
					try
					{
						fun.value()();
					}
					catch(...) {}
				}
			}

			ScopeGuard(ScopeGuard&& rhs) noexcept
			{
				// it is gcc C++17 wooddoo how this object gets constructed
				// but that's fine, becuse it will be swapped with rvalue
				swap(*this, rhs);
				rhs.commit();
			}

			ScopeGuard& operator=(ScopeGuard&& rhs) noexcept
			{
				swap(*this, rhs);
				rhs.commit();
				return *this;
			}

			ScopeGuard() = delete;
			ScopeGuard(const ScopeGuard&) = delete;            // non-copyable
			ScopeGuard& operator=(const ScopeGuard&) = delete; // non-assignable

		protected:
			friend void swap(ScopeGuard& first, ScopeGuard& second) noexcept
			{
				using std::swap;

				swap(static_cast<ScopeGuardBase&>(first), static_cast<ScopeGuardBase&>(second));
				swap(first.fun, second.fun);
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
