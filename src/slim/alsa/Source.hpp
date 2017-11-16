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
			   ~Source();

				Source(Source&& rhs)
				: Source{}
				{
					swap(*this, rhs);
				}

				Source& operator=(Source&& rhs)
				{
					swap(*this, rhs);
					return *this;
				}

				Source(const Source&) = delete;             // non-copyable
				Source& operator=(const Source&) = delete;  // non-assignable

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
				// used only from move constructor
				Source()
				: parameters{"", 0, SND_PCM_FORMAT_UNKNOWN, 0, 0, 0}
				, handlePtr{nullptr}
				, producing{false}
				// TODO: size may not be 0 until assert(^2) is used
				, queuePtr{std::make_unique<RealTimeQueue<Chunk>>(2)} {}

				bool containsData(unsigned char* buffer, snd_pcm_sframes_t frames);
				void copyData(unsigned char* buffer, snd_pcm_sframes_t frames, Chunk& chunk);
				bool restore(snd_pcm_sframes_t error);

				friend void swap(Source& first, Source& second) noexcept
				{
					using std::swap;

					swap(first.parameters, second.parameters);
					swap(first.handlePtr, second.handlePtr);
					swap(first.queuePtr, second.queuePtr);

					// TODO: implement following logic: doAtomically(if producing the pause; swap; if was producing then restart)
					bool t = first.producing;
					first.producing.store(second.producing);
					second.producing = t;
				}

			private:
				Parameters                            parameters;
				snd_pcm_t*                            handlePtr;
				std::atomic<bool>                     producing;
				std::unique_ptr<RealTimeQueue<Chunk>> queuePtr;
		};

		std::ostream& operator<< (std::ostream& os, const alsa::Exception& exception);
	}
}
