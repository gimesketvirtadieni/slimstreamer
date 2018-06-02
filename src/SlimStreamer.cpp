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
#include <cxxopts.hpp>
#include <exception>
#include <fstream>
#include <functional>
#include <g3log/logworker.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "slim/alsa/Parameters.hpp"
#include "slim/alsa/Source.hpp"
#include "slim/conn/tcp/Callbacks.hpp"
#include "slim/conn/tcp/Server.hpp"
#include "slim/conn/udp/Callbacks.hpp"
#include "slim/conn/udp/Server.hpp"
#include "slim/Consumer.hpp"
#include "slim/Container.hpp"
#include "slim/EncoderBase.hpp"
#include "slim/Exception.hpp"
#include "slim/FileConsumer.hpp"
#include "slim/flac/Encoder.hpp"
#include "slim/log/ConsoleSink.hpp"
#include "slim/log/log.hpp"
#include "slim/Pipeline.hpp"
#include "slim/Producer.hpp"
#include "slim/proto/Command.hpp"
#include "slim/proto/Streamer.hpp"
#include "slim/Scheduler.hpp"
#include "slim/util/StreamAsyncWriter.hpp"
#include "slim/wave/Encoder.hpp"


using namespace slim;
using namespace slim::alsa;
using namespace slim::conn;
using namespace slim::proto;
using namespace slim::util;

using TCPCallbacks  = tcp::Callbacks<ContainerBase>;
using TCPConnection = tcp::Connection<ContainerBase>;
using TCPServer     = tcp::Server<ContainerBase>;
using UDPCallbacks  = udp::Callbacks<ContainerBase>;
using UDPServer     = udp::Server<ContainerBase>;


static volatile bool running = true;


void signalHandler(int sig)
{
	running = false;
}


void printVersionInfo()
{
	std::cout << "SlimStreamer version " << VERSION << std::endl;
}


void printLicenseInfo()
{
	printVersionInfo();

	std::cout << std::endl;
	std::cout << "Copyright 2017, Andrej Kislovskij" << std::endl;
	std::cout << std::endl;
	std::cout << "This is PUBLIC DOMAIN software so use at your own risk as it comes" << std::endl;
	std::cout << "with no warranties. This code is yours to share, use and modify without" << std::endl;
	std::cout << "any restrictions or obligations." << std::endl;
	std::cout << std::endl;
	std::cout << "For more information see conwrap/LICENSE or refer refer to http://unlicense.org" << std::endl;
	std::cout << std::endl;
	std::cout << "Author: gimesketvirtadieni at gmail dot com (Andrej Kislovskij)" << std::endl;
	std::cout << std::endl;
	std::cout << "SlimStreamer relies a lot on numerous libraries, which where tailored as required:" << std::endl;
	std::cout << "-> Concurrent Wrapper for asynchronous tasks (Public Domain)" << std::endl;
	std::cout << "-> Logging library 'g3log' (Public Domain)" << std::endl;
	std::cout << "-> Standalone version of 'asio' for networking (Boost Software License)" << std::endl;
	std::cout << "-> Command line options parser 'cxxopts' (MIT)" << std::endl;
	std::cout << "-> Scope guard library 'Boost.ScopeGuard' (Boost Software License 1.0)" << std::endl;
	std::cout << "-> Unit testing library 'googletest' (BSD-3-Clause)" << std::endl;
	std::cout << std::endl;
	std::cout << "Important note: used dependencies may rely on other dependencies shipped with correspondent license." << std::endl;
	std::cout << std::endl;
}


auto createCommandCallbacks(Streamer<TCPConnection>& streamer)
{
	auto callbacksPtr{std::make_unique<TCPCallbacks>()};

	callbacksPtr->setOpenCallback([&](auto& connection)
	{
		streamer.onSlimProtoOpen(connection);
	});
	callbacksPtr->setDataCallback([&](auto& connection, unsigned char* buffer, const std::size_t size)
	{
		streamer.onSlimProtoData(connection, buffer, size);
	});
	callbacksPtr->setCloseCallback([&](auto& connection)
	{
		streamer.onSlimProtoClose(connection);
	});

	return std::move(callbacksPtr);
}


auto createDiscoveryCallbacks()
{
	auto callbacksPtr{std::make_unique<UDPCallbacks>()};

	callbacksPtr->setDataCallback([&](auto& server, unsigned char* buffer, const std::size_t size)
	{
		// simulating LMS by sends 'E' discovery response packet
		server.write("E");
	});

	return std::move(callbacksPtr);
}


auto createPipelines(std::vector<std::unique_ptr<Source>>& sources, Streamer<TCPConnection>& streamer, std::vector<std::unique_ptr<FileConsumer>>& files, EncoderBuilderType encoderBuilder)
{
	std::vector<Pipeline> pipelines;

	for (auto& sourcePtr : sources)
	{
		auto parameters{sourcePtr->getParameters()};

		// TODO: default extension should be provided by encoderBuilder
		//auto streamPtr{std::make_unique<std::ofstream>(std::to_string(parameters.getSamplingRate()) + ".flac", std::ios::binary)};
		//auto writerPtr{std::make_unique<StreamAsyncWriter>(std::move(streamPtr))};
		//auto encoderPtr{std::move(encoderBuilder(parameters.getLogicalChannels(), parameters.getSamplingRate(), parameters.getBitsPerSample(), parameters.getBitsPerValue(), std::ref<AsyncWriter>(*writerPtr), true))};
		//auto filePtr{std::make_unique<FileConsumer>(std::move(writerPtr), std::move(encoderPtr))};

		//pipelines.emplace_back(std::ref<Producer>(*sourcePtr), std::ref<Consumer>(*filePtr));
		//files.push_back(std::move(filePtr));

		pipelines.emplace_back(std::ref<Producer>(*sourcePtr), std::ref<Consumer>(streamer));
	}

	return std::move(pipelines);
}


auto createSources(Parameters parameters)
{
	std::vector<std::tuple<unsigned int, std::string>> rates
	{
		{8000,   "hw:1,1,1"},
		{11025,  "hw:1,1,2"},
		{12000,  "hw:1,1,3"},
		{16000,  "hw:1,1,4"},
		{22500,  "hw:1,1,5"},
		{24000,  "hw:1,1,6"},
		{32000,  "hw:1,1,7"},
		{44100,  "hw:2,1,1"},
		{48000,  "hw:2,1,2"},
		{88200,  "hw:2,1,3"},
		{96000,  "hw:2,1,4"},
		{176400, "hw:2,1,5"},
		{192000, "hw:2,1,6"},
	};

	unsigned int                         chunkDurationMilliSecond{100};
	std::vector<std::unique_ptr<Source>> sources;

	for (auto& rate : rates)
	{
		auto[rateValue, deviceValue] = rate;

		parameters.setSamplingRate(rateValue);
		parameters.setDeviceName(deviceValue);
		parameters.setFramesPerChunk((rateValue * chunkDurationMilliSecond) / 1000);

		sources.push_back(std::make_unique<Source>(parameters,  []
		{
			LOG(ERROR) << LABELS{"slim"} << "Buffer overflow error: a chunk was skipped";
		}));
	}

	return std::move(sources);
}


auto createStreamingCallbacks(Streamer<TCPConnection>& streamer)
{
	auto callbacksPtr{std::make_unique<TCPCallbacks>()};

	callbacksPtr->setOpenCallback([&](auto& connection)
	{
		streamer.onHTTPOpen(connection);
	});
	callbacksPtr->setDataCallback([&](auto& connection, unsigned char* buffer, const std::size_t size)
	{
		streamer.onHTTPData(connection, buffer, size);
	});
	callbacksPtr->setCloseCallback([&](auto& connection)
	{
		streamer.onHTTPClose(connection);
	});

	return std::move(callbacksPtr);
}


int main(int argc, const char *argv[])
{
	// initializing log and adding custom sink
	auto logWorkerPtr = g3::LogWorker::createLogWorker();
	g3::initializeLogging(logWorkerPtr.get());
	g3::only_change_at_initialization::addLogLevel(ERROR);
    logWorkerPtr->addSink(std::make_unique<ConsoleSink>(), &ConsoleSink::print);

    try
	{
		// defining supported options
		cxxopts::Options options("SlimStreamer", "SlimStreamer - A multi-room bit-perfect streamer for systemwise audio\n");
		options
			.custom_help("[options]")
			.add_options()
				("c,maxclients", "Maximum amount of clients able to connect", cxxopts::value<int>()->default_value("10"), "<number>")
				("f,format", "Streaming format", cxxopts::value<std::string>()->default_value("FLAC"), "<PCM|FLAC>")
				("g,gain", "Client audio gain", cxxopts::value<unsigned int>(), "<0-100>")
				("h,help", "Print this help message", cxxopts::value<bool>())
				("l,license", "Print license details", cxxopts::value<bool>())
				("s,slimprotoport", "SlimProto (command connection) server port", cxxopts::value<int>()->default_value("3483"), "<port>")
				("t,httpport", "HTTP (streaming connection) server port", cxxopts::value<int>()->default_value("9000"), "<port>")
				("v,version", "Print version details", cxxopts::value<bool>());

		// parsing provided options
		auto result = options.parse(argc, argv);

		if (result.count("help"))
		{
			std::cout << options.help() << std::endl;
		}
		else if (result.count("license"))
		{
			printLicenseInfo();
		}
		else if (result.count("version"))
		{
			printVersionInfo();
		}
		else
		{
			// setting mandatory parameters
			// TODO: upercase
			auto format        = result["format"].as<std::string>();
			auto httpPort      = result["httpport"].as<int>();
			auto maxClients    = result["maxclients"].as<int>();
			auto slimprotoPort = result["slimprotoport"].as<int>();

			// setting optional parameters
			std::optional<unsigned int> gain{std::nullopt};
			if (result.count("gain"))
			{
				gain = result["gain"].as<unsigned int>();
			}

			// validating parameters and setting format selection
			std::string        pcm{"PCM"};
			std::string        flac{"FLAC"};
			EncoderBuilderType encoderBuilder;
			// TODO: get rid of
			slim::proto::FormatSelection formatSelection;
			if (format == pcm)
			{
				encoderBuilder = [](unsigned int c, unsigned int s, unsigned int bs, unsigned int bv, std::reference_wrapper<AsyncWriter> w, bool h)
				{
					return std::move(std::unique_ptr<EncoderBase>{new wave::Encoder{c, s, bs, bv, w, h}});
				};
				formatSelection = slim::proto::FormatSelection::PCM;
			}
			else if (format == flac)
			{
				encoderBuilder = [](unsigned int c, unsigned int s, unsigned int bs, unsigned int bv, std::reference_wrapper<AsyncWriter> w, bool h)
				{
					return std::move(std::unique_ptr<EncoderBase>{new flac::Encoder{c, s, bs, bv, w, h}});
				};
				formatSelection = slim::proto::FormatSelection::FLAC;
			}
			else
			{
				throw cxxopts::OptionException("Invalid streaming format, only 'FLAC' or 'PCM' values are supported");
			}

			// creating source objects stored in a vector
			Parameters parameters{"", 3, SND_PCM_FORMAT_S32_LE, 0, 128, 0, 8};
			auto sources{createSources(parameters)};

			// Callbacks objects 'glue' SlimProto Streamer with TCP Command Servers
			auto streamerPtr{std::make_unique<Streamer<TCPConnection>>(httpPort, parameters.getLogicalChannels(), parameters.getBitsPerSample(), parameters.getBitsPerValue(), encoderBuilder, gain, formatSelection)};
			auto commandServerPtr{std::make_unique<TCPServer>(slimprotoPort, maxClients, std::move(createCommandCallbacks(*streamerPtr)))};
			auto streamingServerPtr{std::make_unique<TCPServer>(httpPort, maxClients, std::move(createStreamingCallbacks(*streamerPtr)))};
			auto discoveryServerPtr{std::make_unique<UDPServer>(3483, std::move(createDiscoveryCallbacks()))};

			// creating a container for files objects
			std::vector<std::unique_ptr<FileConsumer>> files;

			// creating Scheduler object with destination directed to slimproto Streamer
			auto schedulerPtr
			{
				std::make_unique<Scheduler>(createPipelines(sources, *streamerPtr, files, encoderBuilder))
			};

			// creating Container object within Asio Processor with Scheduler and Servers
			conwrap::ProcessorAsio<ContainerBase> processorAsio
			{
				std::unique_ptr<ContainerBase>
				{
					new Container<Streamer<TCPConnection>, TCPServer, TCPServer, UDPServer, Scheduler>(std::move(streamerPtr), std::move(commandServerPtr), std::move(streamingServerPtr), std::move(discoveryServerPtr), std::move(schedulerPtr))
				}
			};

			// start streaming
			LOG(INFO) << "Starting SlimStreamer...";
			LOG(INFO) << "Streaming format is " << format;
			if (processorAsio.process([](auto context) -> bool
			{
				auto started{false};

				try
				{
					context.getResource()->start();
					started = true;
				}
				catch (const std::exception& error)
				{
					LOG(ERROR) << error.what();
				}
				catch (...)
				{
					std::cout << "Unexpected exception" << std::endl;
				}

				return started;
			}).get())
			{
				LOG(INFO) << "SlimStreamer was started";

				// registering signal handler
				signal(SIGHUP, signalHandler);
				signal(SIGTERM, signalHandler);
				signal(SIGINT, signalHandler);

				// waiting for Control^C
				while(running)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds{200});
				}

				// stop streaming
				LOG(INFO) << "Stopping SlimStreamer...";
				processorAsio.process([](auto context)
				{
					context.getResource()->stop();
				}).wait();
				LOG(INFO) << "SlimStreamer was stopped";
			}
		}
	}
	catch (const cxxopts::OptionException& e)
	{
		std::cout << "Wrong option(s) provided: " << e.what() << std::endl;
	}
	catch (const Exception& error)
	{
		LOG(ERROR) << error;
	}
	catch (const std::exception& error)
	{
		LOG(ERROR) << error.what();
	}
	catch (...)
	{
		std::cout << "Unexpected exception" << std::endl;
	}

	return 0;
}
