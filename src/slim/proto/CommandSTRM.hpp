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
#include <cstring>  // strcpy

#include "slim/proto/Command.hpp"


namespace slim
{
	namespace proto
	{
		struct STRM
		{
			void init()
			{
				opcode[0]     = 's';
				opcode[1]     = 't';
				opcode[2]     = 'r';
				opcode[3]     = 'm';
				command       = 'q';
				autostart     = '0';  // do not autostart
				format        = 'p';  // PCM
				format        = 'p';  // PCM
				pcmSampleSize = '1';  // 16 bit;   it does not mapper here as this is QUIT command
				pcmSampleRate = '3';  // 44.1 kHz; it does not mapper here as this is QUIT command
				pcmChannels   = '2';  // stereo;   it does not mapper here as this is QUIT command
				pcmEndianness = '1';  // WAV;      it does not mapper here as this is QUIT command
			}

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
		};


		class CommandSTRM : public Command<STRM>
		{
			public:
				CommandSTRM()
				{
					strm.init();
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
