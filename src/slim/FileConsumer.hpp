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
#include "slim/EncoderBase.hpp"
#include "slim/log/log.hpp"
#include "slim/util/AsyncWriter.hpp"


namespace slim
{
	class FileConsumer : public Consumer
	{
		public:
			FileConsumer(std::unique_ptr<util::AsyncWriter> w, std::unique_ptr<EncoderBase> e)
			: writerPtr{std::move(w)}
			, encoderPtr{std::move(e)} {}

			virtual ~FileConsumer() = default;
			FileConsumer(const FileConsumer&) = delete;             // non-copyable
			FileConsumer& operator=(const FileConsumer&) = delete;  // non-assignable
			FileConsumer(FileConsumer&& rhs) = delete;              // non-movable
			FileConsumer& operator=(FileConsumer&& rhs) = delete;   // non-assign-movable

			virtual bool consumeChunk(Chunk& chunk) override
			{
				auto* data{chunk.getData()};
				auto  size{chunk.getSize()};

				encoderPtr->encode(data, size);

				LOG(DEBUG) << LABELS{"slim"} << "Written " << chunk.getFrames() << " frames";

				// deferring chunk is irrelevant for a file
				return true;
			}

		private:
			std::unique_ptr<util::AsyncWriter> writerPtr;
			std::unique_ptr<EncoderBase>       encoderPtr;
	};
}
