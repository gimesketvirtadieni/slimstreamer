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
#include <optional>

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
			std::uint16_t gainLeft1;
			std::uint16_t gainLeft2;
			std::uint16_t gainRight1;
			std::uint16_t gainRight2;
			std::uint16_t sequenceID;  // not sure
		};


		struct AUDG
		{
			std::uint16_t size;
			AUDGData      data;
		};
		#pragma pack(pop)


		class CommandAUDG : public Command<AUDG>
		{
			public:
				CommandAUDG(std::optional<unsigned int> gain = std::nullopt)
				{
					memset(&audg, 0, sizeof(AUDG));
					memcpy(&audg.data.opcode, "audg", sizeof(audg.data.opcode));

					if (gain.has_value())
					{
						auto g{gain.value()};
						if (g > 100)
						{
							g = 100;
						}

						audg.data.adjust    = 1;    // digital volume control on/off
						audg.data.preamp    = 255;  // level of preamplication

						// TODO: this is linear scale, logarithmic scale should be used
						audg.data.gainLeft2 = audg.data.gainRight2 = htons(uint32_t{65536 * g} / 100);
					}

					// preparing command size in indianless way
					audg.size = htons(sizeof(audg.data));
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
