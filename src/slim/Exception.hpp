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

#include <sstream>
#include <stdexcept>
#include <string>


namespace slim
{
	class Exception : public std::runtime_error
	{
		public:
			explicit Exception(std::string& e)
			: std::runtime_error{e} {}

			explicit Exception(std::string&& e)
			: std::runtime_error{e} {}

			explicit Exception(const char* e)
			: std::runtime_error{std::string{e}} {}

			// using Rule Of Zero
			virtual ~Exception() = default;
			Exception(const Exception&) = default;             // non-copyable
			Exception& operator=(const Exception&) = default;  // non-assignable
			Exception(Exception&& rhs) = default;
			Exception& operator=(Exception&& rhs) = default;
	};

	std::ostream& operator<< (std::ostream& os, const Exception& exception);
}
