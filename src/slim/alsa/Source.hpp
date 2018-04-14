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
#include <conwrap/ProcessorAsio.hpp>
#include <functional>
#include <memory>

#include "slim/alsa/Parameters.hpp"
#include "slim/Chunk.hpp"
#include "slim/Consumer.hpp"
#include "slim/Producer.hpp"
#include "slim/util/ExpandableBuffer.hpp"
#include "slim/util/RealTimeQueue.hpp"

#include "slim/log/log.hpp"


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
			public:
				Source(Parameters p)
				: parameters{p}
				, handlePtr{nullptr}
				, producing{false}
				, available{false}
				, streaming{true}
				, queuePtr{std::make_unique<util::RealTimeQueue<util::ExpandableBuffer>>(parameters.getQueueSize(), [&](util::ExpandableBuffer& buffer)
				{
					// last channel does not contain PCM data so it will be filtered out
					buffer.capacity(p.getFramesPerChunk() * (p.getChannels() - 1) * (p.getBitsPerSample() >> 3));
				})}
				{
					open();
				}

				virtual ~Source()
				{
					// TODO: it is not safe; a synchronisation with start must be introduced
					if (producing.load(std::memory_order_acquire))
					{
						stop();
					}
					close();
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
					return available.load(std::memory_order_acquire);
				}

				virtual bool isProducing() override
				{
					return producing.load(std::memory_order_acquire);
				}

				virtual bool produce(std::reference_wrapper<Consumer> consumer) override
				{
					// this call does NOT block if bounded queue (buffer) is empty
					return queuePtr->dequeue([&](util::ExpandableBuffer& buffer)
					{
						// creating Chunk object which is a light weight wrapper around ExpandableBuffer with meta data about PCM stream details
						return consumer.get().consume(Chunk{std::ref<util::ExpandableBuffer>(buffer), parameters.getSamplingRate(), parameters.getChannels() - 1, parameters.getBitsPerSample()});
					}
					, [&]
					{
						available.store(false, std::memory_order_release);
					});
				}

				virtual void start(std::function<void()> overflowCallback = [] {}) override;
				virtual void stop(bool gracefully = true) override;

			protected:
				void              close();
				snd_pcm_sframes_t containsData(unsigned char* buffer, snd_pcm_sframes_t frames);
				snd_pcm_sframes_t copyData(unsigned char* srcBuffer, unsigned char* dstBuffer, snd_pcm_sframes_t frames);

				inline auto formatError(std::string message, int error)
				{
					return message + ": name='" + parameters.getDeviceName() + "' error='" + snd_strerror(error) + "'";
				}

				void open();
				bool restore(snd_pcm_sframes_t error);

			private:
				Parameters        parameters;
				snd_pcm_t*        handlePtr;
				std::atomic<bool> producing;
				std::atomic<bool> available;
				bool              streaming;
				std::unique_ptr<util::RealTimeQueue<util::ExpandableBuffer>> queuePtr;
		};
	}
}
