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

#include "slim/util/Buffer.hpp"


namespace slim
{
	namespace proto
	{
		enum CommandSelection
		{
			HELO,
			STRM,
		};


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
		};


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
		};


		class Command
		{
			public:
				Command(CommandSelection commandSelection)
				: buffer{1024}
				{
					strcpy(buffer.data(), "heloo");
				}

				// using Rule Of Zero
			   ~Command() = default;
				Command(const Command&) = delete;             // non-copyable
				Command& operator=(const Command&) = delete;  // non-assignable
				Command(Command&& rhs) = default;
				Command& operator=(Command&& rhs) = default;

				auto getBuffer()
				{
					return buffer.data();
				}

				auto getSize()
				{
					return buffer.size();
				}

			private:
				slim::util::Buffer buffer;
		};
	}
}
