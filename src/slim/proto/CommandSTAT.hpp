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

#include "slim/proto/Command.hpp"


namespace slim
{
	namespace proto
	{
		#pragma pack(push, 1)
		struct STAT
		{
			char          opcode[4];
			std::uint32_t length;
			char          event[4];
			std::uint8_t  numberCRLF;
			std::uint8_t  initializedMAS;
			std::uint8_t  modeMAS;
			std::uint32_t streamBufferSize;
			std::uint32_t streamBufferFullness;
			std::uint32_t bytesReceived1;
			std::uint32_t bytesReceived2;
			std::uint16_t signalStrength;
			std::uint32_t jiffies;
			std::uint32_t outputBufferSize;
			std::uint32_t outputBufferFullness;
			std::uint32_t elapsedSeconds;
			std::uint16_t voltage;
			std::uint32_t elapsedMilliseconds;
			std::uint32_t serverTimestamp;
			std::uint16_t errorCode;
		};
		#pragma pack(pop)


		class CommandSTAT : public Command<STAT>
		{
			public:
				CommandSTAT(unsigned char* buffer, std::size_t size)
				{
					std::string h{"STAT"};
					std::string s{(char*)buffer, h.size()};
					if (h.compare(s))
					{
						throw slim::Exception("Missing 'STAT' label in the header");
					}

					// copying buffer content
					if (size < sizeof(STAT))
					{
						throw slim::Exception("Message is too small");
					}
					memcpy(&stat, buffer, sizeof(STAT));
				}

				// using Rule Of Zero
				virtual ~CommandSTAT() = default;
				CommandSTAT(const CommandSTAT&) = default;
				CommandSTAT& operator=(const CommandSTAT&) = default;
				CommandSTAT(CommandSTAT&& rhs) = default;
				CommandSTAT& operator=(CommandSTAT&& rhs) = default;

				virtual STAT* getBuffer() override
				{
					return &stat;
				}

				virtual std::size_t getSize() override
				{
					return sizeof(STAT);
				}

				inline auto getEvent()
				{
					return std::string{stat.event, sizeof(stat.event)};
				}

				inline static auto isEnoughData(unsigned char* buffer, std::size_t size)
				{
					auto        result{false};
					std::size_t offset{4};

					// TODO: consider max length size
					if (size >= offset + sizeof(std::uint32_t))
					{
						std::uint32_t length{ntohl(*(std::uint32_t*)(buffer + offset))};
						result = (size >= (length + sizeof(STAT::opcode) + sizeof(STAT::length)));
					}

					return result;
				}

			private:
				STAT stat;
		};
	}
}
