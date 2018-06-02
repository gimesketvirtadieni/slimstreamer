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

#include "slim/EncoderBase.hpp"
#include "slim/Exception.hpp"
#include "slim/proto/Command.hpp"
#include "slim/util/AsyncWriter.hpp"


namespace slim
{
	class EncoderBuilder
	{
		using BuilderType = std::function<std::unique_ptr<EncoderBase>(unsigned int, unsigned int, unsigned int, unsigned int, std::reference_wrapper<util::AsyncWriter>, bool)>;

		public:
			EncoderBuilder() = default;
		   ~EncoderBuilder() = default;
			EncoderBuilder(const EncoderBuilder&) = default;
			EncoderBuilder& operator=(const EncoderBuilder&) = default;
			EncoderBuilder(EncoderBuilder&&) = default;
			EncoderBuilder& operator=(EncoderBuilder&&) = default;

			auto getFormat()
			{
				return format;
			}

			// TODO: get rid of parameters
			std::unique_ptr<EncoderBase> build(unsigned int c, unsigned int s, unsigned int bs, unsigned int bv, std::reference_wrapper<util::AsyncWriter> w, bool h)
			{
				if (!builder)
				{
					throw Exception("Builder function was not provided");
				}

				return std::move(builder(c, s, bs, bv, w, h));
			}

			void setBuilder(BuilderType b)
			{
				builder = std::move(b);
			}

			void setFormat(slim::proto::FormatSelection f)
			{
				format = f;
			}

		private:
			BuilderType                  builder{0};
			slim::proto::FormatSelection format{slim::proto::FormatSelection::FLAC};
	};
}
