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

#include "slim/Exception.hpp"
#include "slim/proto/Command.hpp"
#include "slim/util/Buffer.hpp"


namespace slim
{
	namespace proto
	{
		struct HELO
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

					// validating provided data is sufficient and serializing HELO command
					if (size < sizeof(HELO))
					{
						throw slim::Exception("Message is too small");
					}
					memcpy(&helo, buffer, sizeof(HELO));

					// TODO: work in progress
					helo.length = ntohl(helo.length);
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
					// TODO: work in progress
					return helo.length + sizeof(helo.opcode) + sizeof(helo.length);
				}

				inline static auto enoughData(unsigned char* buffer, std::size_t size)
				{
					auto        result{false};
					std::size_t offset{4};

					if (size >= offset + sizeof(std::uint32_t))
					{
						std::uint32_t length{ntohl(*(std::uint32_t*)(buffer + offset))};
						result = (length <= size);
					}

					return result;
				}

			private:
				HELO         helo;
				util::Buffer capabilities;
		};
	}
}
