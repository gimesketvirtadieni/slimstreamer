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
			struct HELO
			{
				char          opcode[4];
				std::uint32_t size;
				std::uint8_t  deviceID;
				std::uint8_t  revision;
				std::uint8_t  mac[6];
				std::uint8_t  uuid[16];
				std::uint16_t wlanChannelList;
				std::uint32_t bytesReceivedHigh;
				std::uint32_t bytesReceivedLow;
				char          language[2];

				inline void convert()
				{
					wlanChannelList   = ntohs(wlanChannelList);
					bytesReceivedHigh = ntohl(bytesReceivedHigh);
					bytesReceivedLow  = ntohl(bytesReceivedLow);
				}
			};
			#pragma pack(pop)


			class CommandHELO : public InboundCommand<HELO>
			{
				public:
					template<typename CommandBufferType>
					CommandHELO(const CommandBufferType& commandBuffer)
					: InboundCommand<HELO>{sizeof(HELO) + 2048, commandBuffer, "HELO"} {}
			};
		}
	}
}
