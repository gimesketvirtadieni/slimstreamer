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
					if (auto index{getFreeBufferIndex()}; index.has_value())
					{
						auto& buffer{buffers[index.value()]};

						// no need for capacity adjustment as it is done by assign method
						buffer.assign(data, size);

						writerPtr->writeAsync(buffer.data(), buffer.size(), [c = callback, &b = buffer](auto& error, auto written)
						{
							// invoking callback
							c(error, written);

							// releasing buffer
							b.size(0);
						});
					}
					// TODO: handle cases when no buffer is available
				}

			protected:
				std::optional<std::size_t> getFreeBufferIndex()
				{
					std::optional<std::size_t> result{std::nullopt};
					auto                       size{buffers.size()};

					for (std::size_t i{0}; i < size; i++)
					{
						if (!buffers[i].size())
						{
							result = i;
							break;
						}
					}

					return result;
				}
			private:
				type_safe::object_ref<Writer>                     writerPtr;
				std::array<util::ExpandableBuffer, TotalElements> buffers;
		};
	}
}
