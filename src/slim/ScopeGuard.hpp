#pragma once

#include <utility>

#include "ScopeGuardBase.hpp"


namespace slim
{
	// Based on https://rnestler.github.io/c-list-of-scopeguard.html
	template<class Fun>
	class ScopeGuard : public ScopeGuardBase
	{
		public:
			ScopeGuard() = delete;

			ScopeGuard(const ScopeGuard&) = delete;

			ScopeGuard(ScopeGuard&& rhs) noexcept
			: ScopeGuardBase{std::move(rhs)}
			, fun{std::move(rhs.fun)} {}

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

			ScopeGuard& operator=(const ScopeGuard&) = delete;

		private:
			Fun fun;
	};


	template<class Fun>
	ScopeGuard<Fun> makeScopeGuard(Fun f)
	{
		return ScopeGuard<Fun>(std::move(f));
	}
}
