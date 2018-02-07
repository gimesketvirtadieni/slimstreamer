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
		#pragma pack(push, 1)
		struct AUDGData
		{
			char          opcode[4];
			std::uint32_t oldGainLeft;
			std::uint32_t oldGainRight;
			std::uint8_t  adjust;
			std::uint8_t  preamp;
			std::uint8_t  gainLeft1;
			std::uint8_t  gainLeft2;
			std::uint8_t  gainLeft3;
			std::uint8_t  gainLeft4;
			std::uint8_t  gainRight1;
			std::uint8_t  gainRight2;
			std::uint8_t  gainRight3;
			std::uint8_t  gainRight4;
			std::uint16_t sequenceID;  // not sure
		};


		struct AUDG
		{
			char     size[2];
			AUDGData data;
		};
		#pragma pack(pop)


		class CommandAUDG : public Command<AUDG>
		{
			public:
				CommandAUDG()
				{
					memset(&audg, 0, sizeof(AUDG));
					memcpy(&audg.data.opcode, "audg", sizeof(audg.data.opcode));

					audg.data.adjust = 1;
					audg.data.preamp = 255;

					// TODO: work in progress
					audg.data.gainLeft3  = 80;
					audg.data.gainLeft4  = 255;
					audg.data.gainRight3 = 80;
					audg.data.gainRight4 = 255;

					// preparing command size in indianless way
					auto size = sizeof(audg.data);
					audg.size[0] = 255 & (size >> 8);
					audg.size[1] = 255 & size;
				}

				// using Rule Of Zero
				virtual ~CommandAUDG() = default;
				CommandAUDG(const CommandAUDG&) = default;
				CommandAUDG& operator=(const CommandAUDG&) = default;
				CommandAUDG(CommandAUDG&& rhs) = default;
				CommandAUDG& operator=(CommandAUDG&& rhs) = default;

				virtual AUDG* getBuffer() override
				{
					return &audg;
				}

				virtual std::size_t getSize() override
				{
					return sizeof(AUDG);
				}

			private:
				AUDG audg;
		};
	}
}
