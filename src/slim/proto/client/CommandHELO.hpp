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
#include "slim/proto/Command.hpp"


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
			};
			#pragma pack(pop)


			class CommandHELO : public Command<HELO>
			{
				public:
					CommandHELO(unsigned char* buffer, std::size_t size)
					{
						// validating there is HELO label in place
						std::string h{"HELO"};
						std::string s{(char*)buffer, h.size()};
						if (h.compare(s))
						{
							throw slim::Exception("Missing 'HELO' label in the header");
						}

						// validating provided data is sufficient for the fixed part of HELO command
						if (size < sizeof(helo))
						{
							throw slim::Exception("Message is too small for the fixed part of HELO command");
						}

						// serializing fixed part of HELO command
						memcpy(&helo, buffer, sizeof(helo));

						// converting command size data
						helo.size = ntohl(helo.size);

						// validating length attribute from HELO command (last -1 accounts for tailing zero)
						if (helo.size > sizeof(helo) + sizeof(capabilities) - sizeof(helo.opcode) - sizeof(helo.size) - 1)
						{
							throw slim::Exception("Length provided in HELO command is too big");
						}

						// making sure there is enough data provided for the dynamic part of HELO command
						if (!Command::isEnoughData(buffer, size))
						{
							throw slim::Exception("Message is too small for HELO command");
						}

						// serializing dynamic part of HELO command
						memcpy(capabilities, buffer + sizeof(helo), helo.size + sizeof(helo.opcode) + sizeof(helo.size) - sizeof(helo));
					}

					// using Rule Of Zero
					virtual ~CommandHELO() = default;
					CommandHELO(const CommandHELO&) = default;
					CommandHELO& operator=(const CommandHELO&) = default;
					CommandHELO(CommandHELO&& rhs) = default;
					CommandHELO& operator=(CommandHELO&& rhs) = default;

					virtual HELO* getBuffer() override
					{
						return &helo;
					}

					virtual std::size_t getSize() override
					{
						return sizeof(helo) + std::strlen(capabilities);
					}

				private:
					HELO helo;
					char capabilities[2048]{0};
			};
		}
	}
}
