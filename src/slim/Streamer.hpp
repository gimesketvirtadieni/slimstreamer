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

#include <conwrap/ProcessorAsioProxy.hpp>
#include <mutex>
#include <thread>

#include "slim/Chunk.hpp"
#include "slim/alsa/Source.hpp"
#include "slim/wave/WAVEFile.hpp"


namespace slim
{
	class Streamer
	{
		public:
			explicit Streamer(alsa::Source source, const char* outputFile);
			        ~Streamer();
			void     consume();
			void     setProcessorProxy(conwrap::ProcessorProxy<Streamer>* p);
			void     start();
			void     stop(bool gracefully = true);

		protected:
			inline bool processChunks(unsigned int maxChunks)
			{
				unsigned int count;
				bool         available;

				for (count = 0, available = true; count < maxChunks && available; count++)
				{
					// this call will NOT block if buffer is empty
					available = source.consume([&](Chunk& chunk)
					{
						this->stream(chunk);
					});
				}

				return available;
			}

			void stream(Chunk& chunk);

		//private:
		public:
			alsa::Source                       source;
			conwrap::ProcessorProxy<Streamer>* processorProxyPtr;
			std::mutex                         lock;
			std::thread                        producerThread;
			std::thread                        consumerThread;
			std::atomic<bool>                  pause;
		    slim::wave::WAVEFile               output;
	};
}
