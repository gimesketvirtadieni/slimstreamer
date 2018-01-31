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
#include <memory>
#include <thread>
#include <vector>

#include "slim/ContainerBase.hpp"
#include "slim/Pipeline.hpp"


namespace slim
{
	template<typename Source, typename Destination>
	class Scheduler
	{
		public:
			explicit Scheduler(std::vector<Pipeline<Source, Destination>> p)
			: pipelines{std::move(p)}
			, processorProxyPtr{nullptr} {}

			// using Rule Of Zero
			~Scheduler() = default;
			Scheduler(const Scheduler&) = delete;             // non-copyable
			Scheduler& operator=(const Scheduler&) = delete;  // non-assignable
			Scheduler(Scheduler&& rhs) = default;
			Scheduler& operator=(Scheduler&& rhs) = default;

			void setProcessorProxy(conwrap::ProcessorProxy<ContainerBase>* p)
			{
				processorProxyPtr = p;
			}

			void start()
			{
				for (auto& pipeline : pipelines)
				{
					// starting producer thread
					std::thread producerThread
					{
						[&]
						{
							LOG(DEBUG) << "Starting PCM data capture thread (id=" << this << ")";

							try
							{
								pipeline.start();
							}
							catch (const slim::Exception& error)
							{
								LOG(ERROR) << error;
							}

							LOG(DEBUG) << "Stopping PCM data capture thread (id=" << this << ")";
						}
					};

					// making sure it is up and running before creating a consumer
					while(producerThread.joinable() && !pipeline.isProducing())
					{
						std::this_thread::sleep_for(std::chrono::milliseconds{10});
					}

					// adding producer thread for Real-Time processing
					threads.push_back(std::move(producerThread));
				}

				// starting a single consumer thread for processing PCM data for all pipelines
				std::thread consumerThread
				{
					[&]
					{
						LOG(DEBUG) << "Starting streamer thread (id=" << this << ")";

						for(auto producing{true}, available{true}; producing;)
						{
							producing = false;
							available = false;

							for (auto& pipeline : pipelines)
							{
								auto p{pipeline.isProducing()};
								auto a{pipeline.isAvailable()};

								// if there is PCM available then submitting a task to the processor
								if (p && a)
								{
									processorProxyPtr->process([&]
									{
										// TODO: calculate maxChunks per processing quantum
										pipeline.onProcess(5);
									});
								}

								// using pipeline status to determine whether to sleep or to exit
								producing |= p;
								available |= a;
							}

							// if no PCM data is available in any of the pipelines then pause processing
							if (!available)
							{
								// TODO: cruise control should be implemented
								std::this_thread::sleep_for(std::chrono::milliseconds{20});
							}
						}

						LOG(DEBUG) << "Stopping streamer thread (id=" << this << ")";
					}
				};

				// saving consumer thread in the same pool
				threads.push_back(std::move(consumerThread));
			}

			void stop(bool gracefully = true)
			{
				// signalling all pipelines to stop processing
				for (auto& pipeline : pipelines)
				{
					try
					{
						pipeline.stop(gracefully);
					}
					catch (const slim::Exception& error)
					{
						LOG(ERROR) << error;
					}
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

		private:
			std::vector<Pipeline<Source, Destination>> pipelines;
			std::vector<std::thread>                   threads;
			conwrap::ProcessorProxy<ContainerBase>*    processorProxyPtr{nullptr};
	};
}
