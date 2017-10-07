#include <chrono>
#include <conwrap/ProcessorAsio.hpp>
#include <exception>
#include <g3log/logworker.hpp>
#include <slim/log/ConsoleSink.hpp>
#include <slim/log/log.hpp>
#include <iostream>
#include <memory>

#include "slim/alsa/Exception.hpp"
#include "slim/alsa/Parameters.hpp"
#include "slim/alsa/Source.hpp"
#include "slim/Chunk.hpp"
#include "slim/Streamer.hpp"


int main(int argc, char *argv[])
{
	// initializing log
	auto logWorkerPtr = g3::LogWorker::createLogWorker();
	g3::initializeLogging(logWorkerPtr.get());
	g3::only_change_at_initialization::setLogLevel(ERROR, true);

	// adding custom sinks
    logWorkerPtr->addSink(std::make_unique<ConsoleSink>(), &ConsoleSink::print);

    try
	{
    	// creating ALSA source object
        slim::alsa::Source source(slim::alsa::Parameters{});

        // creating Streamer object within Processor
        conwrap::ProcessorAsio<slim::Streamer> processorAsio(std::move(std::make_unique<slim::Streamer>(source)));

        // TODO: start/stop should be part of the Streamer API
        processorAsio.getResource()->start();
		std::this_thread::sleep_for(std::chrono::milliseconds{10000});
		processorAsio.getResource()->stop();
	}
	catch (const slim::alsa::Exception& error)
	{
		LOG(ERROR) << error;
	}
	catch (const std::exception& error)
	{
		LOG(ERROR) << error.what();
	}
	catch (...)
	{
		LOG(ERROR) << "Unknown exception";
	}

	return 0;
}
