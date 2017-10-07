#pragma once

#include "slim/ExceptionBase.hpp"


namespace slim
{
	namespace alsa
	{
		class Exception : public slim::ExceptionBase
		{
			public:
				explicit Exception(const char* e, const char* d, const int c)
				: slim::ExceptionBase(e)
				, deviceName(d)
				, code(c) {}

				virtual ~Exception() = default;

				inline const int getCode() const
				{
					return code;
				}

				inline const char* getDeviceName() const
				{
					return deviceName;
				}

				virtual const char* what() const noexcept
				{
			    	return slim::ExceptionBase::what();
				}

			private:
				const char* deviceName;
				const int   code;
		};
	}
}
