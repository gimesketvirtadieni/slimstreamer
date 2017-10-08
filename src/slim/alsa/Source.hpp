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

				inline bool consume(std::function<void(Chunk&)> callback)
				{
					// this call does NOT block if buffer is empty
					return queue.dequeue([&](Chunk& chunk)
					{
						callback(chunk);
					});
				}

				bool isProducing();
				void startProducing(std::function<void()> overflowCallback = []() {});
				void stopProducing(bool gracefully = true);

			protected:
				bool containsData(unsigned char* buffer, snd_pcm_sframes_t frames);
				void copyData(unsigned char* buffer, snd_pcm_sframes_t frames, Chunk& chunk);
				bool restore(snd_pcm_sframes_t error);

			private:
				Parameters           parameters;
				snd_pcm_t*           handlePtr;
				std::atomic<bool>    producing;
				RealTimeQueue<Chunk> queue;
		};

		std::ostream& operator<< (std::ostream& os, const alsa::Exception& exception);
	}
}
