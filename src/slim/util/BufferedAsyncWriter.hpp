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

#include <array>
#include <cstddef>  // std::size_t
#include <functional>
#include <string>
#include <system_error>

#include "slim/util/AsyncWriter.hpp"
#include "slim/util/Buffer.hpp"


namespace slim
{
	namespace util
	{
		template<typename AsyncWriterType, std::size_t TotalElements>
		class BufferedAsyncWriter : public AsyncWriter
		{
			public:
				BufferedAsyncWriter(std::reference_wrapper<AsyncWriterType> w)
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

						if (size <= buffer.getSize())
						{
							// no need to clear data from buffer here as getFreeBufferIndex() picks an empty one
							buffer.addData(data, size);
						}
						else
						{
							buffer = std::move(util::Buffer{data, size});
						}

						writer.get().writeAsync(buffer.getBuffer(), buffer.getDataSize(), [c = callback, &b = buffer](auto error, auto written)
						{
							// invoking callback
							c(error, written);

							// releasing buffer
							b.clear();
						});
					}
					else
					{
						callback(std::make_error_code(std::errc::no_buffer_space), 0);
					}
				}

			protected:
				inline ts::optional<std::size_t> getFreeBufferIndex()
				{
					auto result{ts::optional<std::size_t>{ts::nullopt}};

					for (std::size_t i{0}; i < buffers.size(); i++)
					{
						if (buffers[i].isEmpty())
						{
							result = i;
							break;
						}
					}

					return result;
				}
			private:
				std::reference_wrapper<AsyncWriterType> writer;
				std::array<util::Buffer, TotalElements> buffers;
		};
	}
}
