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
			std::uint8_t  pcmSampleSize;
			std::uint8_t  pcmSampleRate;
			std::uint8_t  pcmChannels;
			std::uint8_t  pcmEndianness;
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

					strm.command       = static_cast<char>(commandSelection);
					strm.autostart     = '0';  // do not autostart
					strm.format        = 'p';  // PCM
					strm.format        = 'p';  // PCM
					strm.pcmSampleSize = '1';  // 16 bit;   it does not matter here as this is QUIT command
					strm.pcmSampleRate = '3';  // 44.1 kHz; it does not matter here as this is QUIT command
					strm.pcmChannels   = '2';  // stereo;   it does not matter here as this is QUIT command
					strm.pcmEndianness = '1';  // WAV;      it does not matter here as this is QUIT command
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
					return sizeof(STRM);
				}

			private:
				STRM strm;
		};
	}
}
