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
#include <functional>
#include <memory>


namespace slim
{
	class EncoderBase
	{
		public:
			EncoderBase() = default;
			virtual ~EncoderBase() = default;
			EncoderBase(const EncoderBase&) = delete;             // non-copyable
			EncoderBase& operator=(const EncoderBase&) = delete;  // non-assignable
			EncoderBase(EncoderBase&&) = delete;                  // non-movable
			EncoderBase& operator=(EncoderBase&&) = delete;       // non-assign-movable

			virtual void        encode(unsigned char* data, const std::size_t size) = 0;
			virtual std::string getMIME() = 0;
	};

	using EncoderBuilderType = std::function<std::unique_ptr<EncoderBase>(unsigned int, unsigned int, unsigned int, unsigned int, std::reference_wrapper<util::AsyncWriter>, bool)>;
}
