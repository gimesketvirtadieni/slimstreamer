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
#include <slim/log/SinkFilter.hpp>


class ConsoleSink : public SinkFilter {
	public:
		     ConsoleSink(std::function<bool(g3::LogMessage&)> = [](g3::LogMessage&) {return false;});
		     ConsoleSink(const ConsoleSink&) = delete;
	        ~ConsoleSink();
		void print(g3::LogMessageMover);
		ConsoleSink& operator=(const ConsoleSink&) = delete;
};
