#pragma once

#include <exception>


namespace slim
{
	class ExceptionBase : public std::exception
	{
		public:
			explicit ExceptionBase(const char* e)
			: error(e) {}

			virtual ~ExceptionBase() = default;

			virtual const char* what() const noexcept
			{
				return error;
			}

		private:
		    const char* error;
	};
}
