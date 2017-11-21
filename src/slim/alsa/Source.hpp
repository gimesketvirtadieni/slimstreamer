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

#include "slim/alsa/Exception.hpp"
#include "slim/alsa/Parameters.hpp"
#include "slim/Chunk.hpp"
#include "slim/RealTimeQueue.hpp"


namespace slim
{
	namespace alsa
	{
		class Source
		{
			public:
				Source(Parameters parameters);

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

						// closing source if it's been opened from the constructor
						if (handlePtr)
						{
							snd_pcm_close(handlePtr);
						}
					}
				}

				Source(const Source&) = delete;             // non-copyable
				Source& operator=(const Source&) = delete;  // non-assignable

				Source(Source&& rhs)
				: parameters{std::move(rhs.parameters)}
				, handlePtr{std::move(rhs.handlePtr)}
				, producing{rhs.producing.load()}  // TODO: handle atopic properly
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
					swap(queuePtr, rhs.queuePtr);

					return *this;
				}

				inline bool consume(std::function<void(Chunk&)> callback)
				{
					// this call does NOT block if buffer is empty
					return queuePtr->dequeue([&](Chunk& chunk)
					{
						callback(chunk);
					});
				}

				Parameters getParameters();
				bool       isProducing();
				void       startProducing(std::function<void()> overflowCallback = []() {});
				void       stopProducing(bool gracefully = true);

			protected:
				bool containsData(unsigned char* buffer, snd_pcm_sframes_t frames);
				void copyData(unsigned char* buffer, snd_pcm_sframes_t frames, Chunk& chunk);
				bool restore(snd_pcm_sframes_t error);

			private:
				// TODO: empty attribute should be refactored to a separate class
				bool                                  empty      = false;
				Parameters                            parameters;
				snd_pcm_t*                            handlePtr;
				std::atomic<bool>                     producing;
				std::unique_ptr<RealTimeQueue<Chunk>> queuePtr;
		};

		std::ostream& operator<< (std::ostream& os, const alsa::Exception& exception);
	}
}
