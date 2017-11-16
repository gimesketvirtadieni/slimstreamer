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
				Parameters(std::string d, unsigned int c, snd_pcm_format_t f, unsigned int r, size_t qs, snd_pcm_uframes_t fc)
				: deviceName{d}
				, channels{c}
				, format{f}
				, rate{r}
				, queueSize{qs}
				, framesPerChunk{fc} {}

			   ~Parameters() = default;

			    Parameters(const Parameters& rhs)
				: deviceName{rhs.deviceName}
				, channels{rhs.channels}
				, format{rhs.format}
				, rate{rhs.rate}
				, queueSize{rhs.queueSize}
				, framesPerChunk{rhs.framesPerChunk} {}

				Parameters& operator=(Parameters rhs)
				{
					swap(*this, rhs);
					return *this;
				}

				Parameters(Parameters&& rhs)
				: Parameters{}
				{
					swap(*this, rhs);
				}

				Parameters& operator=(Parameters&& rhs)
				{
					swap(*this, rhs);
					return *this;
				}

				const int getBitDepth() const
				{
					return snd_pcm_format_physical_width(format);
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
				// used only from move constructor
				Parameters()
				: deviceName{""}
				, channels{0}
				, format{SND_PCM_FORMAT_UNKNOWN}
				, queueSize{0}
				, framesPerChunk{0} {}

				friend void swap(Parameters& first, Parameters& second) noexcept
				{
					using std::swap;

					swap(first.deviceName,     second.deviceName);
					swap(first.channels,       second.channels);
					swap(first.format,         second.format);
					swap(first.rate,           second.rate);
					swap(first.queueSize,      second.queueSize);
					swap(first.framesPerChunk, second.framesPerChunk);
				}

			private:
				// TODO: consider const
				std::string       deviceName;
				unsigned int      channels;
				snd_pcm_format_t  format;
				unsigned int      rate;
				size_t            queueSize;
				snd_pcm_uframes_t framesPerChunk;
		};
	}
}
