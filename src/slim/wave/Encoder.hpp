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
#include <string>

#include "slim/EncoderBase.hpp"


namespace slim
{
	namespace wave
	{
		class Encoder : public EncoderBase
		{
			public:
				explicit Encoder(unsigned int ch, unsigned int bs, unsigned int bv, unsigned int sr, bool hd, std::string ex, std::string mm, EncodedCallbackType ec)
				: EncoderBase{ch, bs, bv, sr, ex, mm, ec} {}

				virtual ~Encoder() = default;
				Encoder(const Encoder&) = delete;             // non-copyable
				Encoder& operator=(const Encoder&) = delete;  // non-assignable
				Encoder(Encoder&&) = delete;                  // non-movable
				Encoder& operator=(Encoder&&) = delete;       // non-assign-movable

				virtual void encode(unsigned char* data, const std::size_t size) override
				{
					samplesEncoded += (size / (getBitsPerSample() >> 3));
					getEncodedCallback()(data, size);
				}

				virtual unsigned long getSamplesEncoded() override
				{
					return samplesEncoded;
				}

			private:
				unsigned long samplesEncoded{0};
		};
	}
}
