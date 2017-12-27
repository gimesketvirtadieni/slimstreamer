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
		struct SETD
		{
			char         opcode[4];
			std::uint8_t id;
		// TODO: clarify if there is an universal way to avoid padding
		} __attribute__((packed));


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
					memcpy(&setd.opcode, "setd", sizeof(setd.opcode));

					setd.id = static_cast<std::uint8_t>(deviceID);
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
