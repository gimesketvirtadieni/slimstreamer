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

#include <cstddef>   // std::size_t
#include <cstdint>   // std::int..._t
#include <functional>
#include <memory>

#include "slim/Chunk.hpp"
#include "slim/Consumer.hpp"
#include "slim/ContainerBase.hpp"
#include "slim/EncoderBase.hpp"
#include "slim/EncoderBuilder.hpp"
#include "slim/log/log.hpp"
#include "slim/util/AsyncWriter.hpp"


namespace slim
{
	class FileConsumer : public Consumer
	{
		public:
			FileConsumer(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> p, std::unique_ptr<util::AsyncWriter> w, EncoderBuilder eb)
			: Consumer{p}
 			, writerPtr{std::move(w)}
 			, headerRequired{eb.getHeader()}
			, samplingRate{eb.getSamplingRate()}
 			{
				eb.setEncodedCallback([&](auto* data, auto size)
				{
					writerPtr->writeAsync(data, size, [](auto error, auto written)
					{
						if (error)
						{
							LOG(ERROR) << LABELS{"slim"} << "Error while writing encoded data: " << error.message();
						}
					});
				});
				encoderPtr = std::move(eb.build());

				if (headerRequired)
				{
					writeHeader();
				}
			}

			virtual ~FileConsumer()
			{
				if (headerRequired)
				{
					writeHeader(bytesWritten);
				}
			}

			FileConsumer(const FileConsumer&) = delete;             // non-copyable
			FileConsumer& operator=(const FileConsumer&) = delete;  // non-assignable
			FileConsumer(FileConsumer&& rhs) = delete;              // non-movable
			FileConsumer& operator=(FileConsumer&& rhs) = delete;   // non-assign-movable

			virtual bool consumeChunk(Chunk& chunk) override
			{
				auto* data{chunk.getData()};
				auto  size{chunk.getSize()};

				encoderPtr->encode(data, size);
				bytesWritten += size;

				LOG(DEBUG) << LABELS{"slim"} << "Written " << chunk.getFrames() << " frames";

				// deferring chunk is irrelevant for a file
				return true;
			}

			inline auto getSamplingRate() const
			{
				return samplingRate;
			}

			virtual bool isRunning() override
			{
				return running;
			}

			virtual void start() override
			{
				running = true;
			}

			virtual void stop(std::function<void()> callback) override
			{
				running = false;
			}

		protected:
			void writeHeader(std::size_t s = 0)
			{
				auto               size{static_cast<std::uint32_t>(s)};
				const unsigned int channels{encoderPtr->getChannels()};
				const unsigned int bitsPerSample{encoderPtr->getBitsPerSample()};
				const unsigned int bytesPerFrame{channels * (bitsPerSample >> 3)};
				const unsigned int byteRate{samplingRate * bytesPerFrame};
				const char         chunkID[]     = {0x52, 0x49, 0x46, 0x46};
				const char         format[]      = {0x57, 0x41, 0x56, 0x45};
				const char         subchunk1ID[] = {0x66, 0x6D, 0x74, 0x20};
				const char         size1[]       = {0x10, 0x00, 0x00, 0x00};
				const char         format1[]     = {0x01, 0x00};  // PCM data = 0x01
				const char         subchunk2ID[] = {0x64, 0x61, 0x74, 0x61};

				// creating header string
				std::stringstream ss;
				ss.write(chunkID, sizeof(chunkID));
				ss.write((const char*)&size, sizeof(size));
				ss.write(format, sizeof(format));
				ss.write(subchunk1ID, sizeof(subchunk1ID));
				ss.write(size1, sizeof(size1));
				ss.write(format1, sizeof(format1));
				ss.write((const char*)&channels, sizeof(std::uint16_t));
				ss.write((const char*)&samplingRate, sizeof(std::uint32_t));
				ss.write((const char*)&byteRate, sizeof(std::uint32_t));
				ss.write((const char*)&bytesPerFrame, sizeof(std::uint16_t));
				ss.write((const char*)&bitsPerSample, sizeof(std::int16_t));
				ss.write(subchunk2ID, sizeof(subchunk2ID));
				ss.write((const char*)&size, sizeof(size));

				// seeking to the beginning
				writerPtr->rewind(0);

				// no need to keep string to be sent as BufferedWriter uses its own buffer for async write
				writerPtr->writeAsync(ss.str(), [&](auto error, auto written)
				{
					if (!error)
					{
						bytesWritten += written;
					}
				});
			}

		private:
			std::unique_ptr<util::AsyncWriter> writerPtr;
			std::unique_ptr<EncoderBase>       encoderPtr;
			bool                               headerRequired;
			unsigned int                       samplingRate;
			bool                               running{false};
			std::size_t                        bytesWritten{0};
	};
}
