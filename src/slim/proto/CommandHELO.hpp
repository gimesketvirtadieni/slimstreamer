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

#include <cstdint>  // std::u..._t types

#include "slim/proto/Command.hpp"


namespace slim
{
	namespace proto
	{
		struct HELOData
		{
			char          opcode[4];
			std::uint32_t length;
			std::uint8_t  deviceID;
			std::uint8_t  revision;
			std::uint8_t  mac[6];
			std::uint8_t  uuid[16];
			std::uint16_t wlanChannelList;
			std::uint32_t bytesReceivedHigh;
			std::uint32_t bytesReceivedLow;
			char          language[2];
		// TODO: clarify if there is an universal way to avoid padding
		} __attribute__((packed));


		struct HELO
		{
			char     size[2];
			HELOData data;
		} __attribute__((packed));


		class CommandHELO : public Command<HELO>
		{
			public:
				CommandHELO()
				{
					// TODO: work in progress
					memset(&helo, 0, sizeof(HELO));
					memcpy(&helo.data.opcode, "helo", sizeof(helo.data.opcode));

					// preparing command size in indianless way
					auto size = sizeof(helo.data);
					helo.size[0] = 255 & (size >> 8);
					helo.size[1] = 255 & size;
				}

				// using Rule Of Zero
				virtual ~CommandHELO() = default;
				CommandHELO(const CommandHELO&) = default;
				CommandHELO& operator=(const CommandHELO&) = delete;
				CommandHELO(CommandHELO&& rhs) = default;
				CommandHELO& operator=(CommandHELO&& rhs) = default;

				virtual HELO* getBuffer() override
				{
					return &helo;
				}

				virtual std::size_t getSize() override
				{
					return sizeof(HELO);
				}

			private:
				HELO helo;
		};
	}
}
