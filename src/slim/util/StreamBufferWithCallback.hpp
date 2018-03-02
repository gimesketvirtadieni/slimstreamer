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

#include <utility>  // std::move


/*
 * This code is based on http://videocortex.io/2017/custom-stream-buffers/
 */
namespace slim
{
	namespace util
	{
		template <typename CallbackType>
		class StreamBufferWithCallback : public std::streambuf
		{
			public:
				StreamBufferWithCallback(CallbackType c)
				: callback{std::move(c ? c : [](const char_type* s, std::streamsize n) {return n;})} {}

				// using Rule Of Zero
				virtual ~StreamBufferWithCallback() = default;
				StreamBufferWithCallback(const StreamBufferWithCallback&) = delete;             // non-copyable
				StreamBufferWithCallback& operator=(const StreamBufferWithCallback&) = delete;  // non-assignable
				StreamBufferWithCallback(StreamBufferWithCallback&& rhs) = default;
				StreamBufferWithCallback& operator=(StreamBufferWithCallback&& rhs) = default;

			protected:
				virtual std::streamsize xsputn(const char_type* s, std::streamsize n) override
				{
					// returning number of characters successfully written
					return callback(s, n);
				};

				virtual int_type overflow(int_type c) override
				{
					// returning number of characters successfully written
					return callback(reinterpret_cast<const char*>(&c), sizeof(char_type));
				}

			private:
				CallbackType callback;
		};


		template <typename CallbackType>
		inline auto makeStreamBufferWithCallback(CallbackType callback)
		{
		    return std::move(StreamBufferWithCallback<CallbackType>(std::move(callback)));
		}
	}
}
