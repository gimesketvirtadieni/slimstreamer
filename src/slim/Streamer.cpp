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

#include <exception>

#include "slim/log/log.hpp"
#include "slim/Streamer.hpp"


namespace slim
{
	Streamer::Streamer(std::vector<Pipeline> p)
	: pipelines{std::move(p)}
	, processorProxyPtr{nullptr} {}


	void Streamer::stream()
	{
		for(auto producing{true}, available{false}; producing;)
		{
			producing = false;
			available = false;

			for (auto& pipeline : pipelines)
			{
				auto task = [&]
				{
					// TODO: calculate maxChunks per processing quantum
					pipeline.processChunks(5);
				};

				// if source is producing PCM data
				if ((producing = pipeline.isProducing()))
				{
					// if there is PCM data ready to be read
					if ((available = pipeline.isAvailable()))
					{
						processorProxyPtr->process(task);
					}
				}
			}

			// if no PCM data is available in any of the pipelines then pause processing
			if (!available)
			{
				// TODO: cruise control should be implemented
				std::this_thread::sleep_for(std::chrono::milliseconds{20});
			}
		}
	}


	void Streamer::setProcessorProxy(conwrap::ProcessorProxy<Streamer>* p)
	{
		processorProxyPtr = p;
	}


	void Streamer::start()
	{
		std::lock_guard<std::mutex> guard{lock};

		for (auto& pipeline : pipelines)
		{
			auto producerTask = [&]
			{
				LOG(DEBUG) << "Starting PCM data capture thread (id=" << this << ")";

				pipeline.start();

				LOG(DEBUG) << "Stopping PCM data capture thread (id=" << this << ")";
			};

			// starting producer and making sure it is up and running before creating a consumer
			std::thread producerThread{producerTask};
			while(producerThread.joinable() && !pipeline.isProducing())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds{10});
			}

			// adding producer thread for Real-Time processing
			threads.emplace_back(std::move(producerThread));
		}

		// creating one single thread for consuming PCM data for all pipelines
		threads.emplace_back(std::thread
		{
			[&]
			{
				LOG(DEBUG) << "Starting streamer thread (id=" << this << ")";

				stream();

				LOG(DEBUG) << "Stopping streamer thread (id=" << this << ")";
			}
		});
	}


	void Streamer::stop(bool gracefully)
	{
		std::lock_guard<std::mutex> guard{lock};

		// signalling all pipelines to stop processing
		for (auto& pipeline : pipelines)
		{
			pipeline.stop(gracefully);
		}

		// waiting for all pipelines' threads to terminate
		for (auto& thread : threads)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
	}
}
