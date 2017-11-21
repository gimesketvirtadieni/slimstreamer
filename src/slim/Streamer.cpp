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
	, processorProxyPtr{nullptr}
	, pause{true} {}


	void Streamer::consume(Pipeline& pipeline)
	{
		// all references captured by this lambda are part of the streamer object
		// it is required to ensure safety: processor will complete all tasks before destroying the streamer
		auto task = [&]()
		{
			try
			{
				// if no more chunks available then requesting pause
				// TODO: calculate max chunks per task
				pause.store(!this->processChunks(pipeline, 5), std::memory_order_release);
			}
			catch (const std::exception& error)
			{
				LOG(ERROR) << error.what();
			}
			catch (...)
			{
				LOG(ERROR) << "Unknown exception";
			}
		};

		// while PCM source is producing data
		for (pause.store(true, std::memory_order_release); pipeline.getSource().isProducing();)
		{
			// keep submitting tasks into the processor
			processorProxyPtr->process(task);

			// if pause is needed
			if (pause.load(std::memory_order_acquire))
			{
				// TODO: cruise control should be implemented
				std::this_thread::sleep_for(std::chrono::milliseconds{100});
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

				pipeline.getSource().startProducing();

				LOG(DEBUG) << "Stopping PCM data capture thread (id=" << this << ")";
			};

			auto consumerTask = [&]
			{
				LOG(DEBUG) << "Starting PCM data consumer thread (id=" << this << ")";

				// TODO: should be moved to a separate class
				consume(pipeline);

				LOG(DEBUG) << "Stopping PCM data consumer thread (id=" << this << ")";
			};

			// starting producer and making sure it is up and running before creating a consumer
			std::thread pt{producerTask};
			while(pt.joinable() && !pipeline.getSource().isProducing())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds{10});
			}

			// adding two threads per pipeline into a vector: producer for Real-Time processing, consumer for streaming
			threads.emplace_back(std::move(pt));
			threads.emplace_back(std::move(std::thread{consumerTask}));
		}
	}


	void Streamer::stop(bool gracefully)
	{
		std::lock_guard<std::mutex> guard{lock};

		// signalling all pipelines to stop processing
		for (auto& pipeline : pipelines)
		{
			pipeline.getSource().stopProducing(gracefully);
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
