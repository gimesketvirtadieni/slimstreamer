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
		struct SETDData
		{
			char         opcode[4];
			std::uint8_t id;
		};


		struct SETD
		{
			char     size[2];
			SETDData data;
		};
		#pragma pack(pop)


		enum class DeviceID : std::uint8_t
		{
			RequestName = 0,
			Squeezebox3 = 4,
		};


		class CommandSETD : public Command<SETD>
		{
			public:
				CommandSETD(DeviceID deviceID)
				{
					memset(&setd, 0, sizeof(SETD));
					memcpy(&setd.data.opcode, "setd", sizeof(setd.data.opcode));

					setd.data.id = static_cast<std::uint8_t>(deviceID);

					// preparing command size in indianless way
					auto size = sizeof(setd.data);
					setd.size[0] = 255 & (size >> 8);
					setd.size[1] = 255 & size;
				}

				// using Rule Of Zero
				virtual ~CommandSETD() = default;
				CommandSETD(const CommandSETD&) = default;
				CommandSETD& operator=(const CommandSETD&) = default;
				CommandSETD(CommandSETD&& rhs) = default;
				CommandSETD& operator=(CommandSETD&& rhs) = default;

				virtual SETD* getBuffer() override
				{
					return &setd;
				}

				virtual std::size_t getSize() override
				{
					return sizeof(SETD);
				}

			private:
				SETD setd;
		};
	}
}
