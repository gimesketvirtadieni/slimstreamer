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

#include <cstddef>  // std::size_t
#include <functional>
#include <ostream>
#include <system_error>

#include "slim/util/AsyncWriter.hpp"
#include "slim/util/StreamBufferWithCallback.hpp"


namespace slim
{
	namespace util
	{
		using StreamBuffer  = util::StreamBufferWithCallback<std::function<std::streamsize(const char*, const std::streamsize)>>;

		class StreamAsyncWriter : public AsyncWriter
		{
			public:
				StreamAsyncWriter(std::unique_ptr<std::ostream> s)
				: streamBuffer{[](auto*, auto size) {return size;}}
				, streamPtr{std::move(s)} {}

				StreamAsyncWriter(std::function<std::streamsize(const char*, const std::streamsize)> callback)
				: streamBuffer{callback}
				, streamPtr{std::make_unique<std::ostream>(&streamBuffer)} {}

				virtual ~StreamAsyncWriter() = default;
				StreamAsyncWriter(const StreamAsyncWriter&) = delete;             // non-copyable
				StreamAsyncWriter& operator=(const StreamAsyncWriter&) = delete;  // non-assignable
				StreamAsyncWriter(StreamAsyncWriter&&) = delete;                  // non-movable
				StreamAsyncWriter& operator=(StreamAsyncWriter&&) = delete;       // non-move-assinagle

				virtual void rewind(const std::streampos pos) override
				{
					streamPtr->seekp(pos);
				}

				// including write overloads
				using AsyncWriter::write;

				virtual std::size_t write(const void* data, const std::size_t size) override
				{
					// std::ostream::write method writes all data before returning, so no need to check how much data was actually written
					streamPtr->write(reinterpret_cast<const char*>(data), size);

					// making sure everything is written
					streamPtr->flush();

					return size;
				}

				// including writeAsync overloads
				using AsyncWriter::writeAsync;

				virtual void writeAsync(const void* data, const std::size_t size, WriteCallback callback = [](auto, auto) {}) override
				{
					try
					{
						write(data, size);
						callback(std::error_code(), size);
					}
					catch(const std::exception& error)
					{
						LOG(ERROR) << error.what();
						callback(std::make_error_code(std::errc::io_error), 0);
					}
				}

			private:
				StreamBuffer                  streamBuffer;
				std::unique_ptr<std::ostream> streamPtr;
		};
	}
}
