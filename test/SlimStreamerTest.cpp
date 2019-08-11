#include <g3log/logworker.hpp>
#include <gtest/gtest.h>
#include <iostream>

#include "slim/log/ConsoleSink.hpp"
#include "slim/log/log.hpp"


int main(int argc, char** argv) {

	// initializing log and adding custom sink
	auto logWorkerPtr = g3::LogWorker::createLogWorker();
	g3::initializeLogging(logWorkerPtr.get());
	g3::only_change_at_initialization::addLogLevel(ERROR);
    logWorkerPtr->addSink(std::make_unique<ConsoleSink>(), &ConsoleSink::print);

	// running all tests
	testing::InitGoogleTest(&argc, argv);
	int return_value = RUN_ALL_TESTS();

	std::cout << "Unit testing was finished" << std::endl;

	return return_value;
}
