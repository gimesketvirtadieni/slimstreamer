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
#include <string>

#include "slim/proto/InboundCommand.hpp"


namespace slim
{
	namespace proto
	{
		namespace client
		{
			#pragma pack(push, 1)
			struct STAT
			{
				char          opcode[4];
				std::uint32_t size;
				char          event[4];
				std::uint8_t  numberCRLF;
				std::uint8_t  initializedMAS;
				std::uint8_t  modeMAS;
				std::uint32_t streamBufferSize;
				std::uint32_t streamBufferFullness;
				std::uint32_t bytesReceived1;
				std::uint32_t bytesReceived2;
				std::uint16_t signalStrength;
				std::uint32_t jiffies;
				std::uint32_t outputBufferSize;
				std::uint32_t outputBufferFullness;
				std::uint32_t elapsedSeconds;
				std::uint16_t voltage;
				std::uint32_t elapsedMilliseconds;
				std::uint32_t serverTimestamp;
				std::uint16_t errorCode;

				inline void convert()
				{
					streamBufferSize     = ntohl(streamBufferSize);
					streamBufferFullness = ntohl(streamBufferFullness);
					bytesReceived1       = ntohl(bytesReceived1);
					bytesReceived2       = ntohl(bytesReceived2);
					signalStrength       = ntohs(signalStrength);
					jiffies              = ntohl(jiffies);
					outputBufferSize     = ntohl(outputBufferSize);
					outputBufferFullness = ntohl(outputBufferFullness);
					elapsedSeconds       = ntohl(elapsedSeconds);
					voltage              = ntohs(voltage);
					elapsedMilliseconds  = ntohl(elapsedMilliseconds);
					serverTimestamp      = ntohl(serverTimestamp);
					errorCode            = ntohs(errorCode);
				}
			};
			#pragma pack(pop)


			class CommandSTAT : public InboundCommand<STAT>
			{
				public:
					template<typename CommandBufferType>
					CommandSTAT(const CommandBufferType& commandBuffer)
					: InboundCommand<STAT>{sizeof(STAT), commandBuffer, "STAT"} {}
			};
		}
	}
}
