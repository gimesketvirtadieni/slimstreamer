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

#include <exception>
#include <functional>
#include <sstream>
#include <string>

#include "slim/alsa/Exception.hpp"
#include "slim/alsa/Source.hpp"
#include "slim/log/log.hpp"
#include "slim/ScopeGuard.hpp"


namespace slim
{
	namespace alsa
	{
		Source::Source(Parameters p)
		: parameters{p}
		, handlePtr{nullptr}
		, producing{false}
		, queuePtr{std::make_unique<RealTimeQueue<Chunk>>(parameters.getQueueSize(), [&](Chunk& chunk)
		{
			// last channel does not contain PCM data so it will be filtered out
			chunk.reset(p.getFramesPerChunk(), p.getChannels() - 1, p.getBitDepth());
		})}
		{
			LOG(DEBUG) << "Creating PCM data source object (id=" << this << ")";

			snd_pcm_hw_params_t* hardwarePtr = nullptr;
			snd_pcm_sw_params_t* softwarePtr = nullptr;
			auto                 deviceName  = parameters.getDeviceName();
			auto                 rate        = parameters.getRate();
			int                  result;
			auto                 guard       = makeScopeGuard([&]()
			{
				// releasing hardware and software parameters
				if (hardwarePtr)
				{
					snd_pcm_hw_params_free(hardwarePtr);
				}
				if (softwarePtr)
				{
					snd_pcm_sw_params_free(softwarePtr);
				}
			});

			if ((result = snd_pcm_open(&handlePtr, deviceName, SND_PCM_STREAM_CAPTURE, 0)) < 0)
			{
				throw alsa::Exception("Cannot open audio device", deviceName, result);
			}
			else if ((result = snd_pcm_hw_params_malloc(&hardwarePtr)) < 0)
			{
				throw alsa::Exception("Cannot allocate hardware parameter structure", deviceName, result);
			}
			else if ((result = snd_pcm_hw_params_any(handlePtr, hardwarePtr)) < 0)
			{
				throw alsa::Exception("Cannot initialize hardware parameter structure", deviceName, result);
			}
			else if ((result = snd_pcm_hw_params_set_access(handlePtr, hardwarePtr, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
			{
				throw alsa::Exception("Cannot set access type", deviceName, result);
			}
			else if ((result = snd_pcm_hw_params_set_format(handlePtr, hardwarePtr, parameters.getFormat())) < 0)
			{
				throw alsa::Exception("Cannot set sample format", deviceName, result);
			}
			else if ((result = snd_pcm_hw_params_set_rate_near(handlePtr, hardwarePtr, &rate, nullptr)) < 0)
			{
				throw alsa::Exception("Cannot set sample rate", deviceName, result);
			}
			else if ((result = snd_pcm_hw_params_set_channels(handlePtr, hardwarePtr, parameters.getChannels())) < 0)
			{
				throw alsa::Exception("Cannot set channel count", deviceName, result);
			}
			else if ((result = snd_pcm_hw_params(handlePtr, hardwarePtr)) < 0)
			{
				throw alsa::Exception("Cannot set hardware parameters", deviceName, result);
			}
			else if ((result = snd_pcm_sw_params_malloc(&softwarePtr)) < 0)
			{
				throw alsa::Exception("Cannot allocate software parameters structure", deviceName, result);
			}
			else if ((result = snd_pcm_sw_params_current(handlePtr, softwarePtr)) < 0)
			{
				throw alsa::Exception("Cannot initialize software parameters structure", deviceName, result);
			}
			else if ((result = snd_pcm_sw_params_set_avail_min(handlePtr, softwarePtr, parameters.getFramesPerChunk())) < 0)
			{
				throw alsa::Exception("Cannot set minimum available count", deviceName, result);
			}

			// start threshold is irrelevant for capturing PCM so just setting to 1
			else if ((result = snd_pcm_sw_params_set_start_threshold(handlePtr, softwarePtr, 1U)) < 0)
			{
				throw alsa::Exception("Cannot set start threshold", deviceName, result);
			}
			else if ((result = snd_pcm_sw_params(handlePtr, softwarePtr)) < 0)
			{
				throw alsa::Exception("Cannot set software parameters", deviceName, result);
			}
		}


		Source::~Source()
		{
			// TODO: it is not safe
			if (producing.load(std::memory_order_acquire))
			{
				stopProducing(false);
			}

			// closing source if it's been opened from the constructor
			if (handlePtr)
			{
				snd_pcm_close(handlePtr);
			}
		}


		// TODO: an iterrator should be introduced that accepts a lambda
		bool Source::containsData(unsigned char* buffer, snd_pcm_sframes_t frames)
		{
			auto contains{false};
			auto bytesPerFrame{parameters.getChannels() * (parameters.getBitDepth() >> 3)};

			for (snd_pcm_sframes_t i = 0; i < frames && !contains; i++)
			{
				auto value = buffer[(i + 1) * bytesPerFrame - 1];  // last byte of the current frame
				if (value)
				{
					contains = true;
				}
			}

			return contains;
		}


		void Source::copyData(unsigned char* srcBuffer, snd_pcm_sframes_t srcFrames, Chunk& chunk)
		{
			auto              bytesPerFrame{parameters.getChannels() * (parameters.getBitDepth() >> 3)};
			auto              dstBuffer{chunk.getBuffer()};
			snd_pcm_sframes_t dstFrames{0};

			// TODO: reuse containsData
			for (snd_pcm_sframes_t i = 0; i < srcFrames; i++)
			{
				auto value = srcBuffer[(i + 1) * bytesPerFrame - 1];  // last byte of the current frame
				if (value)
				{
					// adjusting destination buffer pointer
					dstBuffer = dstBuffer + (parameters.getChannels() - 1) * (parameters.getBitDepth() >> 3);

					// copying byte-by-byte and skyping the last channel
					for (unsigned int j = 0; j < (bytesPerFrame - (parameters.getBitDepth() >> 3)); j++)
					{
						dstBuffer[j] = srcBuffer[i * bytesPerFrame + j];
					}

					// increasing destination frames counter
					dstFrames++;
				}
			}

			// setting new chunk size in terms of frames
			chunk.setFrames(dstFrames);
		}


		bool Source::isProducing()
		{
			return producing.load(std::memory_order_acquire);
		}


		bool Source::restore(snd_pcm_sframes_t error)
		{
			auto restored{true};

			if (error < 0)
			{
				// this is where ALSA stream may be restored depending on an error and the state
				snd_pcm_state(handlePtr);
				restored = false;
			}

			return restored;
		}


		void Source::startProducing(std::function<void()> overflowCallback)
		{
			auto          maxFrames = parameters.getFramesPerChunk();
			unsigned char buffer[maxFrames * parameters.getChannels() * (parameters.getBitDepth() >> 3)];

			// everything inside this try must be real-time safe: no memory allocation, no logging, etc.
			try
			{
				// start receiving data from ALSA
				auto result = snd_pcm_start(handlePtr);
				if (result < 0)
				{
					throw alsa::Exception("Cannot start using audio interface", parameters.getDeviceName(), result);
				}

				// reading ALSA data within this loop
				for (producing.store(true, std::memory_order_release); producing.load(std::memory_order_acquire);)
				{
					// this call will block until buffer is filled or PCM stream state is changed
					snd_pcm_sframes_t frames = snd_pcm_readi(handlePtr, buffer, maxFrames);

					if (frames > 0 && containsData(buffer, frames))
					{
						// reading PCM data directly into the queue
						if (!queuePtr->enqueue([&](Chunk& chunk)
						{
							copyData(buffer, frames, chunk);
						}))
						{
							// calling overflow callback, which must be real-time safe
							overflowCallback();
						}
					}
					else if (frames < 0 && !restore(frames))
					{
						// signaling this loop to exit gracefully
						producing.store(false, std::memory_order_release);

						// if error code is unexpected then breaking this loop (-EBADFD is returned when stopProducing method was called)
						if (frames != -EBADFD)
						{
							throw alsa::Exception("Unexpected error while reading PCM data", parameters.getDeviceName(), frames);
						}
					}
				}
			}
			catch (const alsa::Exception& error)
			{
				LOG(ERROR) << error;
			}
		}


		void Source::stopProducing(bool gracefully)
		{
			if (gracefully)
			{
				snd_pcm_drain(handlePtr);
			}
			else
			{
				snd_pcm_drop(handlePtr);
			}
		}


		std::ostream& operator<< (std::ostream& os, const alsa::Exception& exception)
		{
			return os
			<< exception.what()
			<< " (device='" << exception.getDeviceName() << "', "
			<<   "code="    << exception.getCode() << ", "
			<<   "error='"  << snd_strerror(exception.getCode()) << "')";
		}
	}
}
