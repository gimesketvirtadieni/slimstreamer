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

#include <alsa/asoundlib.h>
#include <atomic>
#include <chrono>
#include <conwrap/ProcessorAsio.hpp>
#include <conwrap/ProcessorProxy.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

#include "slim/alsa/Parameters.hpp"
#include "slim/Chunk.hpp"
#include "slim/Consumer.hpp"
#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"
#include "slim/Producer.hpp"
#include "slim/util/RealTimeQueue.hpp"


namespace slim
{
	namespace alsa
	{
		enum class StreamMarker : unsigned char
		{
			beginningOfStream = 1,
			endOfStream       = 2,
			data              = 3,
		};

		class Source : public Producer
		{
			using TimePointType = std::chrono::time_point<std::chrono::steady_clock>;
			using QueueType     = util::RealTimeQueue<Chunk>;

			public:
				Source(Parameters p, std::function<void()> oc = [] {})
				: parameters{p}
				, overflowCallback{std::move(oc)}
				, queuePtr{std::make_unique<QueueType>(parameters.getQueueSize(), [&](Chunk& chunk)
				{
					// no need to store data from the last channel as it contains commands
					chunk.setCapacity(p.getFramesPerChunk() * p.getLogicalChannels() * (p.getBitsPerSample() >> 3));
				})} {}

				virtual ~Source()
				{
					// it is safe to call stop method multiple times
					stop(false);
				}

				Source(const Source&) = delete;             // non-copyable
				Source& operator=(const Source&) = delete;  // non-assignable
				Source(Source&& rhs) = delete;              // non-movable
				Source& operator=(Source&& rhs) = delete;   // non-move-assignable

				inline auto getParameters()
				{
					return parameters;
				}

				virtual bool isAvailable() override
				{
					bool result;

					if (pauseUntil.has_value() && pauseUntil.value() > std::chrono::steady_clock::now())
					{
						result = false;
					}
					else
					{
						result = available;
						pauseUntil.reset();
					}

					return result;
				}

				virtual bool isRunning() override
				{
					return running;
				}

				virtual bool produceChunk(Consumer& consumer) override
				{
					// if consumer did not accept a chunk then deferring further processing
					if (queuePtr->dequeue([&](Chunk& chunk)
					{
						return consumer.consume(chunk);
					}, [&]
					{
						// TODO: get rid of 'isAvailable'; underFlow callback
						available = false;
					}))
					{
						// TODO: cruise control should be implemented
						pause(50);
					}

					return isAvailable();
				}

				virtual void start() override;
				virtual void stop(bool gracefully = true) override;

			protected:
				void              close();
				snd_pcm_sframes_t containsData(unsigned char* buffer, snd_pcm_uframes_t frames);
				snd_pcm_uframes_t copyData(unsigned char* srcBuffer, unsigned char* dstBuffer, snd_pcm_uframes_t frames);

				inline auto formatError(std::string message, int error = 0)
				{
					return message + ": name='" + parameters.getDeviceName() + (error != 0 ? std::string{"' error='"} + snd_strerror(error) + "'" : "");
				}

				void open();

				void pause(unsigned int millisec)
				{
					pauseUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds{millisec};
				}

				bool restore(snd_pcm_sframes_t error);

			private:
				Parameters                   parameters;
				std::function<void()>        overflowCallback;
				std::unique_ptr<QueueType>   queuePtr;
				snd_pcm_t*                   handlePtr{nullptr};
				std::atomic<bool>            running{false};
				std::atomic<bool>            available{false};
				bool                         streaming{true};
				std::mutex                   lock;
				std::optional<TimePointType> pauseUntil{std::nullopt};
		};
	}
}
