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
#include "slim/util/AsyncWriter.hpp"


namespace slim
{
	class EncoderBuilder
	{
		using BuilderType = std::function<std::unique_ptr<EncoderBase>(unsigned int, unsigned int, unsigned int, unsigned int, std::reference_wrapper<util::AsyncWriter>, bool, std::string, std::string)>;

		public:
			EncoderBuilder() = default;
		   ~EncoderBuilder() = default;
			EncoderBuilder(const EncoderBuilder&) = default;
			EncoderBuilder& operator=(const EncoderBuilder&) = default;
			EncoderBuilder(EncoderBuilder&&) = default;
			EncoderBuilder& operator=(EncoderBuilder&&) = default;

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
					throw Exception("Streaming header flag was not provided");
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

			// TODO: get rid of parameters
			std::unique_ptr<EncoderBase> build(unsigned int c, unsigned int s, unsigned int bs, unsigned int bv, std::reference_wrapper<util::AsyncWriter> w)
			{
				if (!builder)
				{
					throw Exception("Builder function was not provided");
				}
				return std::move(builder(c, s, bs, bv, w, getHeader(), getExtention(), getMIME()));
			}

			void setBuilder(BuilderType b)
			{
				builder = std::move(b);
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

		private:
			BuilderType                                 builder{0};
			std::optional<std::string>                  extention{std::nullopt};
			std::optional<slim::proto::FormatSelection> format{std::nullopt};
			std::optional<bool>                         header{std::nullopt};
			std::optional<std::string>                  mime{std::nullopt};
	};
}
