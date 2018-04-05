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



namespace slim
{
	using WriteCallback = std::function<void(const std::error_code&, const std::size_t)>;

	class StreamWriter
	{
		public:
			virtual ~StreamWriter() = default;

			virtual std::streamsize getBytesWritten()
			{
				return bytesWritten;
			}

			virtual std::size_t write(const void* data, const std::size_t size) = 0;

			virtual void writeAsync(std::string str, WriteCallback callback = [](auto&, auto) {})
			{
				writeAsync(str.c_str(), str.length(), callback);
			}

			virtual void writeAsync(const void* data, const std::size_t size, WriteCallback callback = [](auto&, auto) {}) = 0;

		protected:
			// TODO: get rid of
			std::streamsize bytesWritten{0};
	};
}
