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
#include <system_error>

#include "slim/util/AsyncWriter.hpp"


namespace slim
{
	namespace util
	{
		template<std::size_t TotalElements>
		class BufferedAsyncWriter : public AsyncWriter
		{
			public:
				BufferedAsyncWriter(std::reference_wrapper<AsyncWriter> w)
				: writer{w} {}

				virtual ~BufferedAsyncWriter() = default;
				BufferedAsyncWriter(const BufferedAsyncWriter&) = delete;             // non-copyable
				BufferedAsyncWriter& operator=(const BufferedAsyncWriter&) = delete;  // non-assignable
				BufferedAsyncWriter(BufferedAsyncWriter&&) = delete;                  // non-movable
				BufferedAsyncWriter& operator=(BufferedAsyncWriter&&) = delete;       // non-move-assinagle

				inline bool isBufferAvailable()
				{
					return (getFreeBufferIndex().has_value());
				}

				virtual void rewind(const std::streampos pos)
				{
					writer.get().rewind(pos);
				}

				// including write overloads
				using AsyncWriter::write;

				virtual std::size_t write(const void* data, const std::size_t size)
				{
					return writer.get().write(data, size);
				}

				// including writeAsync overloads
				using AsyncWriter::writeAsync;

				virtual void writeAsync(const void* data, const std::size_t size, WriteCallback callback = [](auto, auto) {}) override
				{
					if (auto index{getFreeBufferIndex()}; index.has_value())
					{
						auto& buffer{buffers[index.value()]};

						// no need for capacity adjustment as it is done by assign method
						buffer.assign(data, size);

						writer.get().writeAsync(buffer.data(), buffer.size(), [c = callback, &b = buffer](auto error, auto written)
						{
							// invoking callback
							c(error, written);

							// releasing buffer
							b.size(0);
						});
					}
					else
					{
						callback(std::make_error_code(std::errc::no_buffer_space), 0);
					}
				}

			protected:
				inline std::optional<std::size_t> getFreeBufferIndex()
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
				std::reference_wrapper<AsyncWriter>               writer;
				std::array<util::ExpandableBuffer, TotalElements> buffers;
		};
	}
}
