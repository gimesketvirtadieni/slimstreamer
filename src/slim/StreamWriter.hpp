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
#include <string>

#include "slim/util/StreamBufferWithCallback.hpp"


namespace slim
{
	using StreamBuffer  = util::StreamBufferWithCallback<std::function<std::streamsize(const char*, const std::streamsize)>>;
	using WriteCallback = std::function<void(const std::error_code&, const std::size_t)>;

	class StreamWriter
	{
		public:
			StreamWriter(std::unique_ptr<std::ostream> s)
			: streamBuffer{[](auto*, auto size) {return size;}}
			, streamPtr{std::move(s)} {}

			StreamWriter(std::function<std::streamsize(const char*, const std::streamsize)> callback)
			: streamBuffer{callback}
			, streamPtr{std::make_unique<std::ostream>(&streamBuffer)} {}

			virtual ~StreamWriter() = default;
			StreamWriter(const StreamWriter&) = delete;             // non-copyable
			StreamWriter& operator=(const StreamWriter&) = delete;  // non-assignable
			StreamWriter(StreamWriter&&) = delete;                  // non-movable
			StreamWriter& operator=(StreamWriter&&) = delete;       // non-move-assinagle

			virtual std::streamsize getBytesWritten()
			{
				return bytesWritten;
			}

			virtual void rewind(const std::streampos pos)
			{
				streamPtr->seekp(pos);
			}

			virtual void write(std::string str)
			{
				write(str.c_str(), str.length());
			}

			virtual void write(const void* data, const std::size_t size)
			{
				// std::ostream::write method writes all data before returning, so no need to check how much data was actually written
				streamPtr->write(reinterpret_cast<const char*>(data), size);
				bytesWritten += size;

				// making sure everything is written
				streamPtr->flush();
			}

			virtual void writeAsync(const void* data, const std::size_t size, WriteCallback callback) = 0;

		private:
			StreamBuffer                  streamBuffer;
			std::unique_ptr<std::ostream> streamPtr;
			std::streamsize               bytesWritten{0};
	};


	class SyncStreamWriter : public StreamWriter
	{
		public:
			SyncStreamWriter(std::unique_ptr<std::ostream> s)
			: StreamWriter{std::move(s)} {}

			SyncStreamWriter(std::function<std::streamsize(const char*, const std::streamsize)> callback)
			: StreamWriter{std::move(callback)} {}

			virtual ~SyncStreamWriter() = default;
			SyncStreamWriter(const SyncStreamWriter&) = delete;             // non-copyable
			SyncStreamWriter& operator=(const SyncStreamWriter&) = delete;  // non-assignable
			SyncStreamWriter(SyncStreamWriter&&) = delete;                  // non-movable
			SyncStreamWriter& operator=(SyncStreamWriter&&) = delete;       // non-move-assinagle

			virtual void writeAsync(const void* data, const std::size_t size, WriteCallback callback) override
			{
				write(data, size);
				// TODO: handle exceptions, it must be exception safe
				callback(std::error_code(), size);
			}
	};
}
