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
#include <cstring>  // memset, memcpy

#include "slim/proto/Command.hpp"


namespace slim
{
	namespace proto
	{
		struct AUDE
		{
			char         opcode[4];
			std::uint8_t enableSPDIF;
			std::uint8_t enableDAC;
		// TODO: clarify if there is an universal way to avoid padding
		} __attribute__((packed));


		class CommandAUDE : public Command<AUDE>
		{
			public:
				CommandAUDE(bool spdif, bool dac)
				{
					memset(&aude, 0, sizeof(AUDE));
					memcpy(&aude.opcode, "aude", sizeof(aude.opcode));

					aude.enableSPDIF = (spdif ? 1 : 0);
					aude.enableDAC   = (dac   ? 1 : 0);
				}

				// using Rule Of Zero
				virtual ~CommandAUDE() = default;
				CommandAUDE(const CommandAUDE&) = default;
				CommandAUDE& operator=(const CommandAUDE&) = default;
				CommandAUDE(CommandAUDE&& rhs) = default;
				CommandAUDE& operator=(CommandAUDE&& rhs) = default;

				virtual AUDE* getBuffer() override
				{
					return &aude;
				}

				virtual std::size_t getSize() override
				{
					return sizeof(AUDE);
				}

			private:
				AUDE aude;
		};
	}
}
