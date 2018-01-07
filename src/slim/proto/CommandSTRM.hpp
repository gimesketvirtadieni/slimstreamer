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
		struct STRMData
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


		struct STRM
		{
			char     size[2];
			STRMData data;
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
					memcpy(&strm.data.opcode, "strm", sizeof(strm.data.opcode));

					strm.data.command    = static_cast<char>(commandSelection);
					strm.data.autostart  = '1';  // autostart
					strm.data.format     = 'p';  // PCM
					strm.data.sampleSize = '3';  // 32 bits per sample
					strm.data.sampleRate = '3';  // 44.1 kHz
					strm.data.channels   = '2';  // stereo
					strm.data.endianness = '1';  // WAV

					if (strm.data.command == static_cast<char>(CommandSelection::Start))
					{
						// TODO: crap
						((unsigned char*)(&strm.data.serverPort))[0] = 35;
						((unsigned char*)(&strm.data.serverPort))[1] = 41;
						std::strcpy(strm.data.httpHeader, "GET /stream.pcm?player=MAC");
					}

					// preparing command size in indianless way
					auto size = getDataSize();
					strm.size[0] = 255 & (size >> 8);
					strm.size[1] = 255 & size;
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
					return sizeof(strm.size) + getDataSize();
				}

			protected:
				std::size_t getDataSize()
				{
					auto size = sizeof(strm.data);

					if (strm.data.command == static_cast<char>(CommandSelection::Start))
					{
						size -= (sizeof(strm.data.httpHeader) - strlen(strm.data.httpHeader));
					}
					else
					{
						size -= sizeof(strm.data.httpHeader);
					}

					return size;
				}

			private:
				STRM strm;
		};
	}
}
