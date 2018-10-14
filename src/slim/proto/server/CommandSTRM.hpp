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

#include "slim/log/log.hpp"
#include "slim/proto/Command.hpp"


namespace slim
{
	namespace proto
	{
		namespace server
		{
			#pragma pack(push, 1)
			struct STRMData
			{
				char          opcode[4];
				char          command;
				std::uint8_t  autostart;
				std::uint8_t  format;
				std::uint8_t  sampleSize;
				std::uint8_t  samplingRate;
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
			};

			struct STRM
			{
				std::uint16_t size;
				STRMData      data;
			};
			#pragma pack(pop)

			class CommandSTRM : public Command<STRM>
			{
				public:
					// this constructor is used in case of CommandSelection::Time, CommandSelection::Stop, etc.
					CommandSTRM(CommandSelection commandSelection)
					: CommandSTRM{commandSelection, FormatSelection::FLAC, 0, 0, {}} {}

					CommandSTRM(CommandSelection commandSelection, std::uint32_t startAt)
					: CommandSTRM{commandSelection, FormatSelection::FLAC, 0, 0, {}}
					{
					}

					CommandSTRM(CommandSelection commandSelection, FormatSelection formatSelection, unsigned int port, unsigned int samplingRate, std::string clientID)
					{
						memset(&strm, 0, sizeof(STRM));
						memcpy(&strm.data.opcode, "strm", sizeof(strm.data.opcode));

						strm.data.command    = static_cast<char>(commandSelection);
						strm.data.autostart  = '0';  // autostart

						if (formatSelection == FormatSelection::PCM)
						{
							strm.data.format       = 'p';  // PCM
							strm.data.sampleSize   = '3';  // 32 bits per sample
							strm.data.samplingRate = mapSamplingRate(samplingRate);
							strm.data.channels     = '2';  // stereo
							strm.data.endianness   = '1';  // WAV
						}
						else if (formatSelection == FormatSelection::FLAC)
						{
							strm.data.format       = 'f';  // FLAC
							strm.data.sampleSize   = '?';  // self-describing
							strm.data.samplingRate = '?';  // self-describing
							strm.data.channels     = '?';  // self-describing
							strm.data.endianness   = '?';  // self-describing
						}

						if (strm.data.command == static_cast<char>(CommandSelection::Start))
						{
							strm.data.serverPort = htons(port);
							std::strcpy(strm.data.httpHeader, (std::string{"GET /stream?player="} += clientID).c_str());
						}

						// preparing command size in indianless way
						strm.size = htons(getDataSize());
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

					// this method use used only in case of WAVE transfer
					char mapSamplingRate(unsigned int samplingRate)
					{
						char result = '?';

						if (samplingRate == 8000)
						{
							result = '5';
						}
						else if (samplingRate == 11025)
						{
							result = '0';
						}
						else if (samplingRate == 12000)
						{
							result = '6';
						}
						else if (samplingRate == 16000)
						{
							result = '7';
						}
						else if (samplingRate == 22500)
						{
							result = '1';
						}
						else if (samplingRate == 24000)
						{
							result = '8';
						}
						else if (samplingRate == 32000)
						{
							result = '2';
						}
						else if (samplingRate == 44100)
						{
							result = '3';
						}
						else if (samplingRate == 48000)
						{
							result = '4';
						}
						else if (samplingRate == 96000)
						{
							result = '9';
						}
						else if (samplingRate > 96000)
						{
							LOG(DEBUG) << LABELS{"proto"} << "SlimProto does not support sampling rate for PCM format beyond 96K";
						}

						return result;
					}

				private:
					STRM strm;
			};
		}
	}
}
