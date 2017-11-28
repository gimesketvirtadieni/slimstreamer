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

#include <functional>
#include <string>

#include "slim/alsa/Source.hpp"
#include "slim/Exception.hpp"
#include "slim/log/log.hpp"
#include "slim/ScopeGuard.hpp"


namespace slim
{
	namespace alsa
	{
		void Source::close()
		{
			// closing source if it's been opened from the constructor
			if (handlePtr)
			{
				snd_pcm_close(handlePtr);
			}
		}


		// TODO: an offset should be returned so containsData & copyData uses only one iterration
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
			auto bytesPerFrame{parameters.getChannels() * (parameters.getBitDepth() >> 3)};
			auto dstBuffer{chunk.getBuffer()};
			auto dstFrames{snd_pcm_sframes_t{0}};

			// TODO: reuse containsData
			for (snd_pcm_sframes_t i = 0; i < srcFrames; i++)
			{
				auto value = srcBuffer[(i + 1) * bytesPerFrame - 1];  // last byte of the current frame
				if (value)
				{
					// copying byte-by-byte and skyping the last channel
					for (unsigned int j = 0; j < (bytesPerFrame - (parameters.getBitDepth() >> 3)); j++)
					{
						dstBuffer[j] = srcBuffer[i * bytesPerFrame + j];
					}

					// adjusting destination buffer pointer
					dstBuffer += (parameters.getChannels() - 1) * (parameters.getBitDepth() >> 3);

					// increasing destination frames counter
					dstFrames++;
				}
			}

			// setting new chunk size
			chunk.setDataSize(dstFrames * (parameters.getChannels() - 1) * (parameters.getBitDepth() >> 3));
		}


		Parameters Source::getParameters()
		{
			return parameters;
		}


		bool Source::isAvailable()
		{
			return available.load(std::memory_order_acquire);
		}


		bool Source::isProducing()
		{
			return producing.load(std::memory_order_acquire);
		}


		void Source::open()
		{
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

			if ((result = snd_pcm_open(&handlePtr, deviceName.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0)
			{
				throw Exception(formatError("Cannot open audio device", result));
			}
			else if ((result = snd_pcm_hw_params_malloc(&hardwarePtr)) < 0)
			{
				throw Exception(formatError("Cannot allocate hardware parameter structure", result));
			}
			else if ((result = snd_pcm_hw_params_any(handlePtr, hardwarePtr)) < 0)
			{
				throw Exception(formatError("Cannot initialize hardware parameter structure", result));
			}
			else if ((result = snd_pcm_hw_params_set_access(handlePtr, hardwarePtr, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
			{
				throw Exception(formatError("Cannot set access type", result));
			}
			else if ((result = snd_pcm_hw_params_set_format(handlePtr, hardwarePtr, parameters.getFormat())) < 0)
			{
				throw Exception(formatError("Cannot set sample format", result));
			}
			else if ((result = snd_pcm_hw_params_set_rate_near(handlePtr, hardwarePtr, &rate, nullptr)) < 0)
			{
				throw Exception(formatError("Cannot set sample rate", result));
			}
			else if ((result = snd_pcm_hw_params_set_channels(handlePtr, hardwarePtr, parameters.getChannels())) < 0)
			{
				throw Exception(formatError("Cannot set channel count", result));
			}
			else if ((result = snd_pcm_hw_params_set_period_size(handlePtr, hardwarePtr, parameters.getFramesPerChunk(), 0)) < 0)
			{
				throw Exception(formatError("Cannot set period size", result));
			}

			// TODO: parametrize
			else if ((result = snd_pcm_hw_params_set_periods(handlePtr, hardwarePtr, 2, 0)) < 0)
			{
				throw Exception(formatError("Cannot set periods count", result));
			}
			else if ((result = snd_pcm_hw_params(handlePtr, hardwarePtr)) < 0)
			{
				throw Exception(formatError("Cannot set hardware parameters", result));
			}
			else if ((result = snd_pcm_sw_params_malloc(&softwarePtr)) < 0)
			{
				throw Exception(formatError("Cannot allocate software parameters structure", result));
			}
			else if ((result = snd_pcm_sw_params_current(handlePtr, softwarePtr)) < 0)
			{
				throw Exception(formatError("Cannot initialize software parameters structure", result));
			}
			else if ((result = snd_pcm_sw_params_set_avail_min(handlePtr, softwarePtr, parameters.getFramesPerChunk())) < 0)
			{
				throw Exception(formatError("Cannot set minimum available count", result));
			}

			// start threshold is irrelevant for capturing PCM so just setting to 1
			else if ((result = snd_pcm_sw_params_set_start_threshold(handlePtr, softwarePtr, 1U)) < 0)
			{
				throw Exception(formatError("Cannot set start threshold", result));
			}
			else if ((result = snd_pcm_sw_params(handlePtr, softwarePtr)) < 0)
			{
				throw Exception(formatError("Cannot set software parameters", result));
			}
		}


		bool Source::restore(snd_pcm_sframes_t error)
		{
			auto restored{true};

			if (error < 0)
			{
				// TODO: this is where ALSA stream may be restored depending on an error and the state
				snd_pcm_state(handlePtr);
				restored = false;
			}

			return restored;
		}


		void Source::startProducing(std::function<void()> overflowCallback)
		{
			auto          maxFrames = parameters.getFramesPerChunk();
			unsigned char srcBuffer[maxFrames * parameters.getChannels() * (parameters.getBitDepth() >> 3)];

			// everything inside this try must be real-time safe: no memory allocation, no logging, etc.
			try
			{
				// start receiving data from ALSA
				auto result = snd_pcm_start(handlePtr);
				if (result < 0)
				{
					throw Exception(formatError("Cannot start using audio device", result));
				}

				// reading ALSA data within this loop
				for (producing.store(true, std::memory_order_release); producing.load(std::memory_order_acquire);)
				{
					// this call will block until buffer is filled or PCM stream state is changed
					snd_pcm_sframes_t frames = snd_pcm_readi(handlePtr, srcBuffer, maxFrames);

					if (frames > 0 && containsData(srcBuffer, frames))
					{
						// reading PCM data directly into the queue
						if (queuePtr->enqueue([&](Chunk& chunk)
						{
							copyData(srcBuffer, frames, chunk);
						}))
						{
							// available is used to provide optimization for a scheduler submitting tasks to a processor
							available.store(true, std::memory_order_release);
						} else {

							// calling overflow callback (which must be real-time safe) in case it was not possible to enqueue a chunk
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
							throw Exception(formatError("Unexpected error while reading PCM data", frames));
						}
					}
				}
			}
			catch (const Exception& error)
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
	}
}
