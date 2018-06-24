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
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"
#include "slim/Pipeline.hpp"


namespace slim
{
	class Scheduler
	{
		public:
			Scheduler(std::vector<Pipeline> p)
			: pipelines{std::move(p)}
			{
				LOG(DEBUG) << LABELS{"slim"} << "Scheduler object was created (id=" << this << ")";
			}

			// using Rule Of Zero
		   ~Scheduler()
			{
				LOG(DEBUG) << LABELS{"slim"} << "Scheduler object was deleted (id=" << this << ")";
			}

			Scheduler(const Scheduler&) = delete;             // non-copyable
			Scheduler& operator=(const Scheduler&) = delete;  // non-assignable
			Scheduler(Scheduler&& rhs) = delete;              // non-movable
			Scheduler& operator=(Scheduler&& rhs) = delete;   // non-move-assignable

			void setProcessorProxy(conwrap::ProcessorProxy<ContainerBase>* p)
			{
				processorProxyPtr = p;
			}

			void start()
			{
				for (auto& pipeline : pipelines)
				{
					// starting PCM data producer thread for Real-Time processing
					std::thread producerThread
					{
						[&]
						{
							LOG(DEBUG) << LABELS{"slim"} << "PCM data capture thread was started (id=" << std::this_thread::get_id() << ")";

							try
							{
								pipeline.getProducer().start();
							}
							catch (const slim::Exception& error)
							{
								LOG(ERROR) << LABELS{"slim"} << "Error in producer thread: " << error;
							}

							LOG(DEBUG) << LABELS{"slim"} << "PCM data capture thread was stopped (id=" << std::this_thread::get_id() << ")";
						}
					};

					// making sure it is up and running before creating a consumer
					while (producerThread.joinable() && !pipeline.getProducer().isRunning())
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
						LOG(DEBUG) << LABELS{"slim"} << "Streamer thread was started (id=" << std::this_thread::get_id() << ")";

						// signalling sheduler when consumer is fully ready
						consumerStarted = true;

						for(auto running{true}, available{true}; running;)
						{
							running   = false;
							available = false;

							for (auto& pipeline : pipelines)
							{
								auto r{pipeline.getProducer().isRunning()};
								auto a{pipeline.getProducer().isAvailable()};

								// if there is PCM available then submitting a task to the processor
								if (r && a)
								{
									processorProxyPtr->process([&producer = pipeline.getProducer(), &consumer = pipeline.getConsumer()]
									{
										// TODO: calculate total chunks per processing quantum
										// processing chunks as long as consumer is not deferring them AND max chunks per task is not reached AND there are chunks available
										auto processed{true};
										for (unsigned int count{5}; processed && count > 0 && producer.isAvailable(); count--)
										{
											processed = producer.produce(consumer);
										}

										// if processing was defered then pausing it for some period
										if (!processed)
										{
											// TODO: cruise control should be implemented
											producer.pause(50);
										}
									});
								}

								// using pipeline status to determine whether to sleep or to exit
								running   |= r;
								available |= a;
							}

							// if no PCM data is available in any of the pipelines then pause processing
							if (!available)
							{
								// TODO: cruise control should be implemented
								std::this_thread::sleep_for(std::chrono::milliseconds{50});
							}
						}

						LOG(DEBUG) << LABELS{"slim"} << "Streamer thread was stopped (id=" << std::this_thread::get_id() << ")";
					}
				};

				// making sure it is up and running
				while(consumerThread.joinable() && !consumerStarted)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds{10});
				}

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
						pipeline.getProducer().stop(gracefully);
					}
					catch (const slim::Exception& error)
					{
						LOG(ERROR) << LABELS{"slim"} << "Error while stopping a pipeline: " << error;
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
			std::vector<Pipeline>                   pipelines;
			std::vector<std::thread>                threads;
			volatile bool                           consumerStarted{false};
			conwrap::ProcessorProxy<ContainerBase>* processorProxyPtr{nullptr};
	};
}
