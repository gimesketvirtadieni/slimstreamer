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
		#pragma pack(push, 1)
		struct RESP
		{
			char          opcode[4];
			std::uint32_t size;
		};
		#pragma pack(pop)


		class CommandRESP : public Command<RESP>
		{
			public:
				CommandRESP(unsigned char* buffer, std::size_t size)
				{
					// validating there is RESP label in place
					std::string h{"RESP"};
					std::string s{(char*)buffer, h.size()};
					if (h.compare(s))
					{
						throw slim::Exception("Missing 'RESP' label in the header");
					}

					// validating provided data is sufficient for the fixed part of RESP command
					if (size < sizeof(resp))
					{
						throw slim::Exception("Message is too small for the fixed part of RESP command");
					}

					// serializing fixed part of RESP command
					memcpy(&resp, buffer, sizeof(resp));

					// TODO: work in progress
					// validating length attribute from RESP command (last -1 accounts for tailing zero)
					//resp.size = ntohl(resp.size);
					//if (resp.size > sizeof(resp) + sizeof(capabilities) - sizeof(resp.opcode) - sizeof(resp.size) - 1)
					//{
					//	throw slim::Exception("Length provided in RESP command is too big");
					//}

					// making sure there is enough data provided for the dynamic part of RESP command
					if (!Command::isEnoughData(buffer, size))
					{
						throw slim::Exception("Message is too small for RESP command");
					}
				}

				// using Rule Of Zero
				virtual ~CommandRESP() = default;
				CommandRESP(const CommandRESP&) = default;
				CommandRESP& operator=(const CommandRESP&) = default;
				CommandRESP(CommandRESP&& rhs) = default;
				CommandRESP& operator=(CommandRESP&& rhs) = default;

				virtual RESP* getBuffer() override
				{
					return &resp;
				}

				virtual std::size_t getSize() override
				{
					return sizeof(resp) + resp.size;
				}

			private:
				RESP resp;
		};
	}
}
