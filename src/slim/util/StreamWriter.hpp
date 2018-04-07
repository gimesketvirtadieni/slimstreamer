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

#include "slim/util/StreamBufferWithCallback.hpp"
#include "slim/util/Writer.hpp"


namespace slim
{
	namespace util
	{
		using StreamBuffer  = util::StreamBufferWithCallback<std::function<std::streamsize(const char*, const std::streamsize)>>;

		class SyncStreamWriter : public Writer
		{
			public:
				SyncStreamWriter(std::unique_ptr<std::ostream> s)
				: streamBuffer{[](auto*, auto size) {return size;}}
				, streamPtr{std::move(s)} {}

				SyncStreamWriter(std::function<std::streamsize(const char*, const std::streamsize)> callback)
				: streamBuffer{callback}
				, streamPtr{std::make_unique<std::ostream>(&streamBuffer)} {}

				virtual ~SyncStreamWriter() = default;
				SyncStreamWriter(const SyncStreamWriter&) = delete;             // non-copyable
				SyncStreamWriter& operator=(const SyncStreamWriter&) = delete;  // non-assignable
				SyncStreamWriter(SyncStreamWriter&&) = delete;                  // non-movable
				SyncStreamWriter& operator=(SyncStreamWriter&&) = delete;       // non-move-assinagle

				virtual void rewind(const std::streampos pos) override
				{
					streamPtr->seekp(pos);
				}

				// including write overloads
				using Writer::write;

				virtual std::size_t write(const void* data, const std::size_t size) override
				{
					// std::ostream::write method writes all data before returning, so no need to check how much data was actually written
					streamPtr->write(reinterpret_cast<const char*>(data), size);

					// making sure everything is written
					streamPtr->flush();

					return size;
				}

				// including writeAsync overloads
				using Writer::writeAsync;

				virtual void writeAsync(const void* data, const std::size_t size, WriteCallback callback = [](auto&, auto) {}) override
				{
					write(data, size);
					// TODO: handle exceptions, it must be exception safe
					callback(std::error_code(), size);
				}

			private:
				StreamBuffer                  streamBuffer;
				std::unique_ptr<std::ostream> streamPtr;
		};
	}
}
