#include <slim/log/log.hpp>
#include <iostream>
#include <exception>

#include "slim/Streamer.hpp"


namespace slim
{
	Streamer::Streamer(alsa::Source& s)
	: source{s}
	, processorProxyPtr{nullptr}
	, pause{true}
	, outputFile{"aaa", std::ios::binary}
	{}


	Streamer::~Streamer()
	{
        outputFile.close();
	}


	void Streamer::consume()
	{
		// all references captured by this lambda are part of the streamer object
		// it is required to ensure safety: processor will complete all tasks before destroying the streamer
		std::function<void(conwrap::Context<Streamer>)> task = [&](auto context)
		{
			try
			{
				// if no more chunks available then requesting pause
				// TODO: calculate max chunks per task
				pause.store(!this->processChunks(5), std::memory_order_release);
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
		for (pause.store(true, std::memory_order_release); source.isProducing();)
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


	void Streamer::setProcessorProxy(conwrap::ProcessorAsioProxy<Streamer>* p)
	{
		processorProxyPtr = p;
	}


	void Streamer::start()
	{
		std::lock_guard<std::mutex> guard{lock};

		// if consumer thread is not running
		if (!consumerThread.joinable())
		{
			// starting a consumer
			consumerThread = std::thread([&]
			{
				LOG(DEBUG) << "Starting PCM data consumer thread (id=" << this << ")";

				// TODO: should be moved to a separate class
				consume();

				LOG(DEBUG) << "Stopping PCM data consumer thread (id=" << this << ")";
			});
		}

		if (!producerThread.joinable())
		{
			// starting a producer
			producerThread = std::thread([&]
			{
				LOG(DEBUG) << "Starting PCM data capture thread (id=" << this << ")";

				source.startProducing();

				LOG(DEBUG) << "Stopping PCM data capture thread (id=" << this << ")";
			});
		}
	}


	void Streamer::stop(bool gracefully)
	{
		std::lock_guard<std::mutex> guard{lock};

		// if producer thread is running
		if (producerThread.joinable())
		{
			source.stopProducing(gracefully);

			// waiting for producer thread to complete
			producerThread.join();
		}

		// if consumer thread is running
		if (consumerThread.joinable())
		{
			// waiting for consumer thread to complete
			consumerThread.join();
		}
	}


	void Streamer::stream(Chunk& chunk)
	{
	    auto buffer = chunk.getBuffer();
	    auto size   = chunk.getSize();

	    if (outputFile.is_open())
	    {
		    for (unsigned int i = 0; i < size; i++)
	        {
				auto value = (char)buffer[i];
				outputFile.write(&value, sizeof(char));
	        }
	    }
	}
}
