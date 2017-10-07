#include <slim/log/ConsoleSink.hpp>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>



ConsoleSink::ConsoleSink(std::function<bool(g3::LogMessage&)> filter) :
	SinkFilter(filter) {
}


ConsoleSink::~ConsoleSink() {
}


void ConsoleSink::print(g3::LogMessageMover logEntry) {
	auto logMessage = logEntry.get();

	if (!filter(logMessage)) {

		// TODO: SinkFormatter should be introduced
		std::string s = logMessage.message();
		std::cout << logMessage.timestamp("%Y/%m/%d %H:%M:%S.%f3") << " "
				  << logMessage.level()
				  << " ["  << logMessage.threadID() << "]"
				  << " ("  << logMessage.file() << ":" << logMessage.line() << ")"
				  << (logMessage._labels.size() > 0 ? " {" + logMessage.labels() + "}" : "")
				  << " - " << rightTrim(logMessage.message())
				  << std::endl << std::flush;
	}
}
