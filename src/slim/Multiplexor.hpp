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

#pragma once

#include <conwrap/ProcessorProxy.hpp>
#include <functional>  // reference_wrapper
#include <thread>
#include <vector>

#include "slim/ContainerBase.hpp"


namespace slim
{
	template <class ProducerType>
	class Multiplexor : public Producer
	{
		public:
			Multiplexor(std::vector<std::unique_ptr<ProducerType>> p)
			: producers{std::move(p)} {}

			// using Rule Of Zero
			virtual ~Multiplexor() = default;
			Multiplexor(const Multiplexor&) = delete;             // non-copyable
			Multiplexor& operator=(const Multiplexor&) = delete;  // non-assignable
			Multiplexor(Multiplexor&& rhs) = delete;              // non-movable
			Multiplexor& operator=(Multiplexor&& rhs) = delete;   // non-move-assignable

			virtual bool isAvailable() override
			{
				return true;
			}

			virtual bool isRunning() override
			{
				return true;
			}

			virtual void pause(unsigned int millisec) override {}

			virtual bool produce(Consumer&) override
			{
				return true;
			}

			virtual void setProcessorProxy(conwrap::ProcessorProxy<ContainerBase>* p) override {}

			virtual void start() override
			{
				for (auto& producerPtr : producers)
				{
					// starting PCM data producer thread for Real-Time processing
					std::thread producerThread
					{
						[&producer = *producerPtr]
						{
							LOG(DEBUG) << LABELS{"slim"} << "PCM data capture thread was started (id=" << std::this_thread::get_id() << ")";

							try
							{
								producer.start();
							}
							catch (const Exception& error)
							{
								LOG(ERROR) << LABELS{"slim"} << "Error in producer thread: " << error;
							}
							catch (const std::exception& error)
							{
								LOG(ERROR) << LABELS{"slim"} << "Error in producer thread: " << error.what();
							}

							LOG(DEBUG) << LABELS{"slim"} << "PCM data capture thread was stopped (id=" << std::this_thread::get_id() << ")";
						}
					};

					// making sure it is up and running
					while (producerThread.joinable() && !producerPtr->isRunning())
					{
						std::this_thread::sleep_for(std::chrono::milliseconds{1});
					}

					// adding producer thread for Real-Time processing
					threads.push_back(std::move(producerThread));
				}
			}

			virtual void stop(bool gracefully = true) override
			{
				// signalling all threads to stop processing
				for (auto& producerPtr : producers)
				{
					try
					{
						producerPtr->stop(gracefully);
					}
					catch (const slim::Exception& error)
					{
						LOG(ERROR) << LABELS{"slim"} << "Error while stopping a producer: " << error;
					}
				}

				// waiting for all threads to terminate
				for (auto& thread : threads)
				{
					if (thread.joinable())
					{
						thread.join();
					}
				}
			}

		/* TODO: temporary hack; must be private */
		public:
			std::vector<std::unique_ptr<ProducerType>> producers;
			std::vector<std::thread>                   threads;
	};
}
