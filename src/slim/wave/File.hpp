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

#include <memory>

#include "slim/Chunk.hpp"
#include "slim/Consumer.hpp"
#include "slim/log/log.hpp"
#include "slim/StreamWriter.hpp"
#include "slim/wave/WAVEStream.hpp"


namespace slim
{
	namespace wave
	{
		class File : public Consumer
		{
			public:
				File(std::unique_ptr<StreamWriter> w, unsigned int channels, unsigned int sampleRate, unsigned int bitsPerSample)
				: writerPtr{std::move(w)}
				, waveStream{writerPtr.get(), channels, sampleRate, bitsPerSample}
				, bytesPerFrame{channels * (bitsPerSample >> 3)}
				{
					waveStream.writeHeader();
				}

				// there is a need for a custom destructor so Rule Of Zero cannot be used
				// Instead of The Rule of The Big Four (and a half) the following approach is used: http://scottmeyers.blogspot.dk/2014/06/the-drawbacks-of-implementing-move.html
				virtual ~File()
				{
					waveStream.writeHeader(waveStream.getBytesWritten());
				}

				File(const File&) = delete;             // non-copyable
				File& operator=(const File&) = delete;  // non-assignable
				File(File&& rhs) = delete;              // non-movable
				File& operator=(File&& rhs) = delete;   // non-assign-movable

				virtual bool consume(Chunk chunk) override
				{
					auto* data{chunk.getBuffer().data()};
					auto  size{chunk.getBuffer().size()};

					waveStream.write(data, size);

					LOG(DEBUG) << LABELS{"wave"} << "Written " << (size / bytesPerFrame) << " frames";

					// deferring chunk is irrelevant for WAVE file destination
					return true;
				}

			private:
				std::unique_ptr<StreamWriter> writerPtr;
				WAVEStream                    waveStream;
				unsigned int                  bytesPerFrame;
		};
	}
}
