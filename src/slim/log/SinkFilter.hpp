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

#include <functional>
#include <g3log/logmessage.hpp>


// TODO: this should be in a formatter file
std::string rightTrim(const std::string&);


class SinkFilter {
	public:
		SinkFilter(std::function<bool(g3::LogMessage&)> = NULL);
	   ~SinkFilter();
		bool filter(g3::LogMessage&);

	private:
		std::function<bool(g3::LogMessage&)> _filter;
};
