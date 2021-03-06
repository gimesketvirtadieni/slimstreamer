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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <conwrap2/Processor.hpp>
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
#include <type_safe/optional.hpp>
#include <vector>

#include "slim/alsa/Parameters.hpp"
#include "slim/alsa/Source.hpp"
#include "slim/conn/tcp/Callbacks.hpp"
#include "slim/conn/tcp/Server.hpp"
#include "slim/conn/udp/Callbacks.hpp"
#include "slim/conn/udp/Server.hpp"
#include "slim/Consumer.hpp"
#include "slim/Container.hpp"
#include "slim/Demultiplexor.hpp"
#include "slim/EncoderBase.hpp"
#include "slim/EncoderBuilder.hpp"
#include "slim/Exception.hpp"
#include "slim/FileConsumer.hpp"
#include "slim/flac/Encoder.hpp"
#include "slim/log/ConsoleSink.hpp"
#include "slim/log/log.hpp"
#include "slim/Multiplexor.hpp"
#include "slim/proto/OutboundCommand.hpp"
#include "slim/proto/Streamer.hpp"
#include "slim/Scheduler.hpp"
#include "slim/util/StreamAsyncWriter.hpp"
#include "slim/util/Timestamp.hpp"
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


static std::atomic<bool> running{true};


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
	callbacksPtr->setDataCallback([&](auto& connection, unsigned char* buffer, const std::size_t size, const slim::util::Timestamp timestamp)
	{
		streamer.onSlimProtoData(connection, buffer, size, timestamp);
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


auto createFileConsumers(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy, std::vector<std::unique_ptr<Source>>& producers, EncoderBuilder& encoderBuilder)
{
	// creating a container for files objects
	std::vector<std::unique_ptr<FileConsumer>> fileConsumers;

	std::for_each(producers.begin(), producers.end(), [&](auto& producerPtr)
	{
		// creating a file writer
		auto parameters{producerPtr->getParameters()};
		auto streamPtr{std::make_unique<std::ofstream>(std::to_string(parameters.getSamplingRate()) + "." + encoderBuilder.getExtention(), std::ios::binary)};
		auto writerPtr{std::make_unique<StreamAsyncWriter>(std::move(streamPtr))};

		// creating an encoder for writing to files
		encoderBuilder.setChannels(parameters.getLogicalChannels());
		encoderBuilder.setSamplingRate(parameters.getSamplingRate());
		encoderBuilder.setBitsPerSample(parameters.getBitsPerSample());
		encoderBuilder.setBitsPerValue(parameters.getBitsPerValue());

		fileConsumers.emplace_back(std::make_unique<FileConsumer>(processorProxy, std::move(writerPtr), encoderBuilder));
	});

	return fileConsumers;
}


auto createProducers(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> processorProxy, Parameters parameters)
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
	std::vector<std::unique_ptr<Source>> producers;

	for (auto& [rate, device] : rates)
	{
		parameters.setSamplingRate(rate);
		parameters.setDeviceName(device);
		parameters.setFramesPerChunk((rate * chunkDurationMilliSecond) / 1000);

		producers.push_back(std::make_unique<Source>(processorProxy, parameters, []
		{
			LOG(ERROR) << LABELS{"slim"} << "Buffer overflow error: a chunk was skipped";
		}));
	}

	return std::move(producers);
}


auto createStreamingCallbacks(Streamer<TCPConnection>& streamer)
{
	auto callbacksPtr{std::make_unique<TCPCallbacks>()};

	callbacksPtr->setOpenCallback([&](auto& connection)
	{
		streamer.onHTTPOpen(connection);
	});
	callbacksPtr->setDataCallback([&](auto& connection, unsigned char* buffer, const std::size_t size, const slim::util::Timestamp timestamp)
	{
		streamer.onHTTPData(connection, buffer, size);
	});
	callbacksPtr->setCloseCallback([&](auto& connection)
	{
		streamer.onHTTPClose(connection);
	});

	return std::move(callbacksPtr);
}


int main(int argc, char *argv[])
{
	// initializing log and adding custom sink
	auto logWorkerPtr = g3::LogWorker::createLogWorker();
	g3::initializeLogging(logWorkerPtr.get());
	g3::only_change_at_initialization::addLogLevel(ERROR);
    logWorkerPtr->addSink(std::make_unique<ConsoleSink>(), &ConsoleSink::print);

	// this flag is used to wait for streamer to stop
	// it is updated by the processor's thread so it has to be in a 'outer' scope so that it can 'out-live' the processor
	auto schedulerRunning{true};

    try
	{
		// defining supported options
		cxxopts::Options options("SlimStreamer", "SlimStreamer - A multi-room bit-perfect streamer for systemwise audio\n");
		options
			.custom_help("[options]")
			.add_options()
				("c,maxclients", "Maximum amount of clients able to connect", cxxopts::value<int>()->default_value("10"), "<number>")
				("F,files", "Dump PCM to files", cxxopts::value<bool>())
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
			auto gain{type_safe::optional<unsigned int>{0}};
			gain.reset();
			if (result.count("gain"))
			{
				gain = result["gain"].as<unsigned int>();
			}

			// validating parameters and setting format selection
			std::string    pcm{"PCM"};
			std::string    flac{"FLAC"};
			EncoderBuilder encoderBuilder;
			encoderBuilder.setHeader(false);
			if (format == pcm)
			{
				encoderBuilder.setBuilder([](unsigned int ch, unsigned int bs, unsigned int bv, unsigned int sr, bool hd, std::string ex, std::string mm, std::function<void(unsigned char*, std::size_t)> ec)
				{
					return std::move(std::unique_ptr<EncoderBase>{new wave::Encoder{ch, bs, bv, sr, hd, ex, mm, ec}});
				});
				encoderBuilder.setFormat(slim::proto::FormatSelection::PCM);
				encoderBuilder.setExtention("wav");
				encoderBuilder.setMIME("audio/x-wave");
			}
			else if (format == flac)
			{
				encoderBuilder.setBuilder([](unsigned int ch, unsigned int bs, unsigned int bv, unsigned int sr, bool hd, std::string ex, std::string mm, std::function<void(unsigned char*, std::size_t)> ec)
				{
					return std::move(std::unique_ptr<EncoderBase>{new flac::Encoder{ch, bs, bv, sr, hd, ex, mm, ec}});
				});
				encoderBuilder.setFormat(slim::proto::FormatSelection::FLAC);
				encoderBuilder.setExtention("flac");
				encoderBuilder.setMIME("audio/flac");
			}
			else
			{
				throw cxxopts::OptionException("Invalid streaming format, only 'FLAC' or 'PCM' values are supported");
			}

			// creating 'template' parameters
			Parameters parameters{"", 3, SND_PCM_FORMAT_S32_LE, 0, 128, 0, 8};

			// pre-configuring an encoder builder
			encoderBuilder.setChannels(parameters.getLogicalChannels());
			encoderBuilder.setBitsPerSample(parameters.getBitsPerSample());
			encoderBuilder.setBitsPerValue(parameters.getBitsPerValue());

			// TODO: streamer is not owned by any container in case when PCM is directed to files; consider a better way
			std::unique_ptr<Streamer<TCPConnection>> streamerPtr;

			// creating Container object within Processor with Scheduler and Servers
			conwrap2::Processor<std::unique_ptr<ContainerBase>> processor{[&](auto processorProxy)
			{
				// creating producers (one per device)
				auto producers{createProducers(processorProxy, parameters)};

				// creating a multiplexor which combines producers into one 'virtual' producer
				auto multiplexorPtr{std::make_unique<Multiplexor<Source>>(processorProxy, std::move(producers))};

				// creating a streamer object
				streamerPtr = std::move(std::make_unique<Streamer<TCPConnection>>(processorProxy, httpPort, encoderBuilder, gain));

				// Callbacks objects 'glue' SlimProto Streamer with TCP Command Servers
				auto commandServerPtr
				{
					std::make_unique<TCPServer>(processorProxy, slimprotoPort, maxClients, std::move(createCommandCallbacks(*streamerPtr)))
				};
				auto streamingServerPtr
				{
					std::make_unique<TCPServer>(processorProxy, httpPort, maxClients, std::move(createStreamingCallbacks(*streamerPtr)))
				};
				auto discoveryServerPtr
				{
					std::make_unique<UDPServer>(processorProxy, 3483, std::move(createDiscoveryCallbacks()))
				};

				// choosing consumer based on parameters provided
				std::unique_ptr<Consumer> consumerPtr;
				if (result.count("files"))
				{
					if (encoderBuilder.getFormat() == slim::proto::FormatSelection::PCM)
					{
						encoderBuilder.setHeader(true);
					}
					consumerPtr = std::make_unique<Demultiplexor<FileConsumer>>(processorProxy, std::move(createFileConsumers(processorProxy, producers, encoderBuilder)));
				}
				else
				{
					consumerPtr = std::move(streamerPtr);
				}

				// creating a scheduler
				auto schedulerPtr{std::make_unique<Scheduler<Multiplexor<Source>, Consumer>>(processorProxy, std::move(multiplexorPtr), std::move(consumerPtr))};

				return std::move(std::unique_ptr<ContainerBase>
				{
					new Container<TCPServer, TCPServer, UDPServer, Scheduler<Multiplexor<Source>, Consumer>>(processorProxy, std::move(commandServerPtr), std::move(streamingServerPtr), std::move(discoveryServerPtr), std::move(schedulerPtr))
				});
			}};
			LOG(INFO) << "Streaming format is " << format;

			// start streaming
			processor.process([](auto& context)
			{
				try
				{
					LOG(INFO) << "Starting SlimStreamer...";
					context.getResource()->start();
					LOG(INFO) << "SlimStreamer was started";
				}
				catch (const std::exception& error)
				{
					LOG(ERROR) << error.what();
				}
				catch (...)
				{
					std::cout << "Unexpected exception" << std::endl;
				}
			});

			// registering signal handler
			signal(SIGHUP, signalHandler);
			signal(SIGTERM, signalHandler);
			signal(SIGINT, signalHandler);

			// waiting for Control^C
			while (running)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds{500});
			}

			// stop streaming
			LOG(INFO) << "Stopping SlimStreamer...";
			processor.process([](auto& context)
			{
				context.getResource()->stop();
			});

			// waiting for stop to complete
			while (schedulerRunning)
			{
				// TODO: processWithDelay should be used instead, however memory sanitizer claims a problem
				std::this_thread::sleep_for(std::chrono::milliseconds{50});

				processor.process([&](auto& context)
				{
					schedulerRunning = context.getResource()->isSchedulerRunning();
				});
			}
			LOG(INFO) << "SlimStreamer was stopped";
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
