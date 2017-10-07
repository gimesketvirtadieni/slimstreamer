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
