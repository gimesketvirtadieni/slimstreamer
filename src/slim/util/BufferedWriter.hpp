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
#include <string>
#include <type_safe/reference.hpp>

#include "slim/util/Writer.hpp"


namespace slim
{
	namespace util
	{
		template<std::size_t TotalElements>
		class BufferedWriter : public Writer
		{
			public:
				BufferedWriter(type_safe::object_ref<Writer> w)
				: writerPtr{w} {}

				virtual ~BufferedWriter() = default;
				BufferedWriter(const BufferedWriter&) = delete;             // non-copyable
				BufferedWriter& operator=(const BufferedWriter&) = delete;  // non-assignable
				BufferedWriter(BufferedWriter&&) = delete;                  // non-movable
				BufferedWriter& operator=(BufferedWriter&&) = delete;       // non-move-assinagle

				virtual void rewind(const std::streampos pos)
				{
					writerPtr->rewind(pos);
				}

				// including write overloads
				using Writer::write;

				virtual std::size_t write(const void* data, const std::size_t size)
				{
					return writerPtr->write(data, size);
				}

				// including writeAsync overloads
				using Writer::writeAsync;

				virtual void writeAsync(const void* data, const std::size_t size, WriteCallback callback = [](auto&, auto) {}) override
				{
					writerPtr->writeAsync(data, size, callback);
				}

			private:
				type_safe::object_ref<Writer>                     writerPtr;
				std::array<util::ExpandableBuffer, TotalElements> buffers;
		};
	}
}
