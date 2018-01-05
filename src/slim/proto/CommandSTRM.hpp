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
		struct STRM
		{
			char          opcode[4];
			char          command;
			std::uint8_t  autostart;
			std::uint8_t  format;
			std::uint8_t  sampleSize;
			std::uint8_t  sampleRate;
			std::uint8_t  channels;
			std::uint8_t  endianness;
			std::uint8_t  threshold;
			std::uint8_t  spdifEnable;
			std::uint8_t  transitionPeriod;
			std::uint8_t  transitionType;
			std::uint8_t  flags;
			std::uint8_t  outputThreshold;
			std::uint8_t  slaves;
			std::uint32_t replayGain;
			std::uint16_t serverPort;
			std::uint32_t serverIP;

			char          httpHeader[64];
		// TODO: clarify if there is an universal way to avoid padding
		} __attribute__((packed));


		enum class CommandSelection : char
		{
			Start = 's',
			Stop  = 'q',
		};


		class CommandSTRM : public Command<STRM>
		{
			public:
				CommandSTRM(CommandSelection commandSelection)
				{
					memset(&strm, 0, sizeof(STRM));
					memcpy(&strm.opcode, "strm", sizeof(strm.opcode));

					strm.command    = static_cast<char>(commandSelection);
					strm.autostart  = '1';  // autostart
					strm.format     = 'p';  // PCM
					strm.sampleSize = '3';  // 32 bits per sample
					strm.sampleRate = '3';  // 44.1 kHz
					strm.channels   = '2';  // stereo
					strm.endianness = '1';  // WAV

					if (strm.command == static_cast<char>(CommandSelection::Start))
					{
						// TODO: crap
						((unsigned char*)(&strm.serverPort))[0] = 35;
						((unsigned char*)(&strm.serverPort))[1] = 41;
						std::strcpy(strm.httpHeader, "GET /stream.pcm?player=MAC");
					}
				}

				// using Rule Of Zero
				virtual ~CommandSTRM() = default;
				CommandSTRM(const CommandSTRM&) = default;
				CommandSTRM& operator=(const CommandSTRM&) = default;
				CommandSTRM(CommandSTRM&& rhs) = default;
				CommandSTRM& operator=(CommandSTRM&& rhs) = default;

				virtual STRM* getBuffer() override
				{
					return &strm;
				}

				virtual std::size_t getSize() override
				{
					return sizeof(strm) - (strm.command == static_cast<char>(CommandSelection::Start) ? (sizeof(strm.httpHeader) - strlen(strm.httpHeader)) : sizeof(strm.httpHeader));
				}

			private:
				STRM strm;
		};
	}
}
