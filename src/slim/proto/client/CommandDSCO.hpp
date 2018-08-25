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
			struct DSCO
			{
				char          opcode[4];
				std::uint32_t size;
			};
			#pragma pack(pop)


			class CommandDSCO : public Command<DSCO>
			{
				public:
					CommandDSCO(unsigned char* buffer, std::size_t size)
					{
						// validating there is DSCO label in place
						std::string h{"DSCO"};
						std::string s{(char*)buffer, h.size()};
						if (h.compare(s))
						{
							throw slim::Exception("Missing 'DSCO' label in the header");
						}

						// validating provided data is sufficient for the fixed part of DSCO command
						if (size < sizeof(dsco))
						{
							throw slim::Exception("Message is too small for DSCO command");
						}

						// serializing fixed part of DSCO command
						memcpy(&dsco, buffer, sizeof(dsco));

						// converting command size data
						dsco.size = ntohl(dsco.size);

						// TODO: work in progress
						// validating length attribute from DSCO command (last -1 accounts for tailing zero)
						//if (dsco.size > sizeof(dsco) + sizeof(capabilities) - sizeof(dsco.opcode) - sizeof(dsco.size) - 1)
						//{
						//	throw slim::Exception("Length provided in DSCO command is too big");
						//}

						// making sure there is enough data provided for the dynamic part of DSCO command
						if (!Command::isEnoughData(buffer, size))
						{
							throw slim::Exception("Message is too small for DSCO command");
						}
					}

					// using Rule Of Zero
					virtual ~CommandDSCO() = default;
					CommandDSCO(const CommandDSCO&) = default;
					CommandDSCO& operator=(const CommandDSCO&) = default;
					CommandDSCO(CommandDSCO&& rhs) = default;
					CommandDSCO& operator=(CommandDSCO&& rhs) = default;

					virtual DSCO* getBuffer() override
					{
						return &dsco;
					}

					virtual std::size_t getSize() override
					{
						return sizeof(dsco) + dsco.size;
					}

				private:
					DSCO dsco;
			};
		}
	}
}
