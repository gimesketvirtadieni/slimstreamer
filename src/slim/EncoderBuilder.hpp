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
#include <optional>
#include <string>

#include "slim/EncoderBase.hpp"
#include "slim/Exception.hpp"
#include "slim/proto/Command.hpp"


namespace slim
{
	class EncoderBuilder
	{
		using BuilderType = std::function<std::unique_ptr<EncoderBase>(unsigned int, unsigned int, unsigned int, unsigned int, bool, std::string, std::string, std::function<void(unsigned char*, std::size_t)>)>;

		public:
			EncoderBuilder() = default;
		   ~EncoderBuilder() = default;
			EncoderBuilder(const EncoderBuilder&) = default;
			EncoderBuilder& operator=(const EncoderBuilder&) = default;
			EncoderBuilder(EncoderBuilder&&) = default;
			EncoderBuilder& operator=(EncoderBuilder&&) = default;

			auto getBitsPerSample()
			{
				if (!bitsPerSample.has_value())
				{
					throw Exception("Bits-per-sample parameter was not provided");
				}
				return bitsPerSample.value();
			}

			auto getBitsPerValue()
			{
				if (!bitsPerValue.has_value())
				{
					throw Exception("Bits-per-value parameter was not provided");
				}
				return bitsPerValue.value();
			}

			auto getEncodedCallback()
			{
				if (!encodedCallback.has_value())
				{
					throw Exception("Encoded data callback was not provided");
				}
				return encodedCallback.value();
			}

			auto getChannels()
			{
				if (!channels.has_value())
				{
					throw Exception("Number of channels was not provided");
				}
				return channels.value();
			}

			auto getExtention()
			{
				if (!extention.has_value())
				{
					throw Exception("Default file extention was not provided");
				}
				return extention.value();
			}

			auto getFormat()
			{
				if (!format.has_value())
				{
					throw Exception("Streaming format was not provided");
				}
				return format.value();
			}

			auto getHeader()
			{
				if (!header.has_value())
				{
					throw Exception("Streaming header parameter was not provided");
				}
				return header.value();
			}

			auto getMIME()
			{
				if (!mime.has_value())
				{
					throw Exception("Streaming format MIME type was not provided");
				}
				return mime.value();
			}

			auto getSamplingRate()
			{
				if (!samplingRate.has_value())
				{
					throw Exception("Sampling rate was not provided");
				}
				return samplingRate.value();
			}

			std::unique_ptr<EncoderBase> build()
			{
				if (!builder)
				{
					throw Exception("Builder function was not provided");
				}
				return std::move(builder(getChannels(), getBitsPerSample(), getBitsPerValue(), getSamplingRate(), getHeader(), getExtention(), getMIME(), getEncodedCallback()));
			}

			void setBitsPerSample(unsigned int bs)
			{
				bitsPerSample = bs;
			}

			void setBitsPerValue(unsigned int bv)
			{
				bitsPerValue = bv;
			}

			void setBuilder(BuilderType b)
			{
				builder = std::move(b);
			}

			void setChannels(unsigned int c)
			{
				channels = c;
			}

			void setEncodedCallback(std::function<void(unsigned char*, std::size_t)> ec)
			{
				encodedCallback = ec;
			}

			void setExtention(std::string e)
			{
				extention = e;
			}

			void setFormat(slim::proto::FormatSelection f)
			{
				format = f;
			}

			void setHeader(bool h)
			{
				header = h;
			}

			void setMIME(std::string m)
			{
				mime = m;
			}

			void setSamplingRate(unsigned int s)
			{
				samplingRate = s;
			}

		private:
			BuilderType                                                     builder{0};
			std::optional<unsigned int>                                     channels{std::nullopt};
			std::optional<unsigned int>                                     samplingRate{std::nullopt};
			std::optional<unsigned int>                                     bitsPerSample{std::nullopt};
			std::optional<unsigned int>                                     bitsPerValue{std::nullopt};
			std::optional<std::string>                                      extention{std::nullopt};
			std::optional<slim::proto::FormatSelection>                     format{std::nullopt};
			std::optional<bool>                                             header{std::nullopt};
			std::optional<std::string>                                      mime{std::nullopt};
			std::optional<std::function<void(unsigned char*, std::size_t)>> encodedCallback{std::nullopt};
	};
}
