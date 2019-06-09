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
#include <cstdint>  // std::u..._t types
#include <cstring>  // std::strlen
#include <string>

#include "slim/Exception.hpp"
#include "slim/proto/InboundCommand.hpp"


namespace slim
{
	namespace proto
	{
		namespace client
		{
			#pragma pack(push, 1)
			struct SETD
			{
				char          opcode[4];
				std::uint32_t size;
				std::uint8_t  id;

				inline void convert() {}
			};
			#pragma pack(pop)


			class CommandSETD : public InboundCommand<SETD>
			{
				public:
					template<typename CommandBufferType>
					CommandSETD(const CommandBufferType& commandBuffer)
					: InboundCommand<SETD>{sizeof(SETD), commandBuffer, "SETD"} {}
			};
		}
	}
}
