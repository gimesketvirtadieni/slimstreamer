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

#include <functional>
#include <memory>

#include "slim/Chunk.hpp"
#include "slim/Consumer.hpp"
#include "slim/log/log.hpp"
#include "slim/util/Writer.hpp"


namespace slim
{
	template <typename EncoderType>
	class FileConsumer : public Consumer
	{
		public:
			FileConsumer(std::unique_ptr<util::Writer> w, unsigned int channels, unsigned int sampleRate, unsigned int bitsPerSample)
			: writerPtr{std::move(w)}
			, encoder{channels, sampleRate, bitsPerSample, std::ref<util::Writer>(*writerPtr), true} {}

			virtual ~FileConsumer() = default;
			FileConsumer(const FileConsumer&) = delete;             // non-copyable
			FileConsumer& operator=(const FileConsumer&) = delete;  // non-assignable
			FileConsumer(FileConsumer&& rhs) = delete;              // non-movable
			FileConsumer& operator=(FileConsumer&& rhs) = delete;   // non-assign-movable

			virtual bool consume(Chunk chunk) override
			{
				auto* data{chunk.getData()};
				auto  size{chunk.getSize()};

				encoder.encode(data, size);

				LOG(DEBUG) << LABELS{"slim"} << "Written " << chunk.getFrames() << " frames";

				// deferring chunk is irrelevant for a file
				return true;
			}

		private:
			std::unique_ptr<util::Writer> writerPtr;
			EncoderType                   encoder;
	};
}
