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
#include "slim/RealTimeQueue.hpp"

/* TODO: use enum */
#define BEGINNING_OF_STREAM_MARKER 1
#define END_OF_STREAM_MARKER       2
#define DATA_MARKER                3


namespace slim
{
	namespace alsa
	{
		class Source
		{
			public:
				Source(Parameters p)
				: parameters{p}
				, handlePtr{nullptr}
				, producing{false}
				, available{false}
				, streaming{true}
				, queuePtr{std::make_unique<RealTimeQueue<Chunk>>(parameters.getQueueSize(), [&](Chunk& chunk)
				{
					// last channel does not contain PCM data so it will be filtered out
					chunk.setSize(p.getFramesPerChunk() * (p.getChannels() - 1) * (p.getBitDepth() >> 3));
				})}
				{
					open();
				}

				// there is a need for a custom destructor so Rule Of Zero cannot be used
				// Instead of The Rule of The Big Four (and a half) the following approach is used: http://scottmeyers.blogspot.dk/2014/06/the-drawbacks-of-implementing-move.html
				~Source()
				{
					if (!empty)
					{
						// TODO: it is not safe
						if (producing.load(std::memory_order_acquire))
						{
							stopProducing(false);
						}
						close();
					}
				}

				Source(const Source&) = delete;             // non-copyable
				Source& operator=(const Source&) = delete;  // non-assignable

				Source(Source&& rhs)
				: parameters{std::move(rhs.parameters)}
				, handlePtr{std::move(rhs.handlePtr)}
				, producing{rhs.producing.load()}  // TODO: handle atopic properly
				, available{rhs.available.load()}  // TODO: handle atopic properly
				, streaming{rhs.streaming}
				, queuePtr{std::move(rhs.queuePtr)}
				{
					rhs.empty = true;
				}

				Source& operator=(Source&& rhs)
				{
					using std::swap;

					// any resources should be released for this object here because it will take over resources from rhs object
					if (!empty)
					{
						// TODO: it is not safe
						if (producing.load(std::memory_order_acquire))
						{
							stopProducing(false);
						}

						// closing source if it's been opened from the constructor
						if (handlePtr)
						{
							snd_pcm_close(handlePtr);
						}
					}
					rhs.empty = true;

					swap(parameters, rhs.parameters);
					swap(handlePtr, rhs.handlePtr);

					// TODO: handle atomic properly
					producing.store(rhs.producing.load());
					available.store(rhs.available.load());
					swap(streaming, rhs.streaming);
					swap(queuePtr, rhs.queuePtr);

					return *this;
				}

				inline bool isAvailable()
				{
					return available.load(std::memory_order_acquire);
				}

				inline bool isProducing()
				{
					return producing.load(std::memory_order_acquire);
				}

				void startProducing(std::function<void()> overflowCallback = []() {});
				void stopProducing(bool gracefully = true);

				inline bool supply(std::function<void(Chunk&)> consumer)
				{
					// this call does NOT block if buffer is empty
					auto consumed = queuePtr->dequeue([&](Chunk& chunk)
					{
						consumer(chunk);
					});

					// updating available attribute used to optimize the scheduler
					if (!consumed)
					{
						available.store(false, std::memory_order_release);
					}

					return consumed;
				}

			protected:
				void              close();
				snd_pcm_sframes_t containsData(unsigned char* buffer, snd_pcm_sframes_t frames);
				void              copyData(unsigned char* buffer, snd_pcm_sframes_t frames, Chunk& chunk);

				inline auto formatError(std::string message, int error)
				{
					return message + ": name='" + parameters.getDeviceName() + "' error='" + snd_strerror(error) + "'";
				}

				void open();
				bool restore(snd_pcm_sframes_t error);

			private:
				// TODO: empty attribute should be refactored to a separate class
				bool                                  empty      = false;
				Parameters                            parameters;
				snd_pcm_t*                            handlePtr;
				std::atomic<bool>                     producing;
				std::atomic<bool>                     available;
				bool                                  streaming;
				std::unique_ptr<RealTimeQueue<Chunk>> queuePtr;
		};
	}
}
