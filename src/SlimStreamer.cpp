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

#include <chrono>
#include <conwrap/ProcessorAsio.hpp>
#include <csignal>
#include <exception>
#include <g3log/logworker.hpp>
#include <memory>

#include "slim/alsa/Exception.hpp"
#include "slim/alsa/Parameters.hpp"
#include "slim/alsa/Source.hpp"
#include "slim/Chunk.hpp"
#include "slim/log/ConsoleSink.hpp"
#include "slim/log/log.hpp"
#include "slim/Streamer.hpp"


static volatile bool running = true;

void signalHandler(int sig)
{
	running = false;
}


int main(int argc, char *argv[])
{
	// initializing log
	auto logWorkerPtr = g3::LogWorker::createLogWorker();
	g3::initializeLogging(logWorkerPtr.get());
	g3::only_change_at_initialization::setLogLevel(ERROR, true);

	// adding custom sinks
    logWorkerPtr->addSink(std::make_unique<ConsoleSink>(), &ConsoleSink::print);

    signal(SIGHUP, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGINT, signalHandler);

	try
	{
        // creating Streamer object with ALSA Parameters within Processor
		conwrap::ProcessorAsio<slim::Streamer> processorAsio
		{
        	slim::alsa::Source{slim::alsa::Parameters{"hw:1,1,7", 3, SND_PCM_FORMAT_S32_LE, 44100, 128, 1024}},
			"aaa.wav"
		};

        auto streamer2 = slim::Streamer
		{
        	slim::alsa::Source{slim::alsa::Parameters{"hw:2,1,1", 3, SND_PCM_FORMAT_S32_LE, 48000, 128, 1024}},
			"bbb.wav"
		};

        // start streaming
        processorAsio.getResource()->start();

        streamer2.processorProxyPtr = processorAsio.getProcessorProxy();
        streamer2.start();

        // waiting for Control^C
        while(running)
        {
			std::this_thread::sleep_for(std::chrono::milliseconds{200});
        }

		// stop streaming
        streamer2.stop();
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
