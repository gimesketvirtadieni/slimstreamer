#pragma once



namespace slim
{
	// Based on https://rnestler.github.io/c-list-of-scopeguard.html
	class ScopeGuardBase
	{
		public:
			ScopeGuardBase()
			: active{true} {}

			ScopeGuardBase(const ScopeGuardBase&) = delete;

			ScopeGuardBase(ScopeGuardBase&& rhs)
			: active{rhs.active}
			{
				rhs.commit();
			}

			inline void commit() noexcept
			{
				active = false;
			}

			inline bool isActive() noexcept
			{
				return active;
			}

			ScopeGuardBase& operator=(const ScopeGuardBase&) = delete;

		protected:
			~ScopeGuardBase() = default;

		private:
			bool active;
	};
}
