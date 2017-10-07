#pragma once

#include <alsa/asoundlib.h>
#include <cstddef>


namespace slim
{
	namespace alsa
	{
		class Parameters
		{
			public:
				const unsigned int getBitDepth() const
				{
					return 16;
				}

				const unsigned int getChannels() const
				{
					return 3;
				}

				const char* getDeviceName() const
				{
					return "hw:CARD=Loopback,DEV=1,1";
				}

				const size_t getQueueSize() const
				{
					// TODO: calculate based on latency
					return 64;
				}

				const unsigned int getRate() const
				{
					return 48000;
				}

				const snd_pcm_format_t getFormat() const
				{
					return SND_PCM_FORMAT_S16_LE;
				}

				const snd_pcm_uframes_t getFramesPerChunk() const
				{
					return 1024;
				}
		};
	}
}
