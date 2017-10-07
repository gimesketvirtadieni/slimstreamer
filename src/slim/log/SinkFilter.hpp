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
