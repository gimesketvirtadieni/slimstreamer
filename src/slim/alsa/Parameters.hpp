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

#include <alsa/asoundlib.h>
#include <cstddef>
#include <memory>
#include <string>


namespace slim
{
	namespace alsa
	{
		class Parameters
		{
			public:
				Parameters() = default;
			   ~Parameters() = default;
				Parameters(const Parameters&) = default;
				Parameters& operator=(const Parameters&) = default;
				Parameters(Parameters&&) = default;
				Parameters& operator=(Parameters&&) = default;

				const unsigned int getBitDepth() const
				{
					return bitDepth;
				}

				const unsigned int getChannels() const
				{
					return channels;
				}

				const char* getDeviceName() const
				{
					return deviceName.c_str();
				}

				const size_t getQueueSize() const
				{
					// TODO: calculate based on latency
					return queueSize;
				}

				const unsigned int getRate() const
				{
					return rate;
				}

				const snd_pcm_format_t getFormat() const
				{
					return format;
				}

				const snd_pcm_uframes_t getFramesPerChunk() const
				{
					return framesPerChunk;
				}

			protected:
				friend void swap(Parameters& first, Parameters& second) noexcept
				{
					unsigned int      t1;
					std::string       t2;
					size_t            t3;
					snd_pcm_format_t  t4;
					snd_pcm_uframes_t t5;

					// TODO: reuse swap
					t1              = first.bitDepth;
					first.bitDepth  = second.bitDepth;
					second.bitDepth = t1;

					t1              = first.channels;
					first.channels  = second.channels;
					second.channels = t1;

					t2                = first.deviceName;
					first.deviceName  = second.deviceName;
					second.deviceName = t2;

					t3               = first.queueSize;
					first.queueSize  = second.queueSize;
					second.queueSize = t3;

					t1          = first.rate;
					first.rate  = second.rate;
					second.rate = t1;

					t4            = first.format;
					first.format  = second.format;
					second.format = t4;

					t5                    = first.framesPerChunk;
					first.framesPerChunk  = second.framesPerChunk;
					second.framesPerChunk = t5;
				}

			private:
				// TODO: consider const
				unsigned int      bitDepth       = 16;
				unsigned int      channels       = 3;
				std::string       deviceName     = "hw:CARD=Loopback,DEV=1,1";
				size_t            queueSize      = 128;
				unsigned int      rate           = 48000;
				snd_pcm_format_t  format         = SND_PCM_FORMAT_S16_LE;
				snd_pcm_uframes_t framesPerChunk = 1024;
		};
	}
}
