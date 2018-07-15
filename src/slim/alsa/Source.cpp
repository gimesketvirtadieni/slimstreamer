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

#include <scope_guard.hpp>
#include <string>

#include "slim/alsa/Source.hpp"
#include "slim/Exception.hpp"


namespace slim
{
	namespace alsa
	{
		void Source::close()
		{
			int result;

			if ((result = snd_pcm_close(handlePtr)) < 0)
			{
				throw Exception(formatError("Error while closing audio device", result));
			}
		}


		snd_pcm_sframes_t Source::containsData(unsigned char* buffer, snd_pcm_uframes_t frames)
		{
			auto offset{-1};
			auto bytesPerFrame{parameters.getTotalChannels() * (parameters.getBitsPerSample() >> 3)};

			for (snd_pcm_uframes_t i = 0; i < frames && offset < 0; i++)
			{
				// processing PCM data marker
				StreamMarker value{buffer[(i + 1) * bytesPerFrame - 1]};  // last byte of the current frame
				if (value == StreamMarker::beginningOfStream)
				{
					streaming = true;
				}
				else if (value == StreamMarker::endOfStream)
				{
					streaming = false;
				}
				else if (value == StreamMarker::data && streaming)
				{
					offset = i;
				}
			}

			return offset;
		}


		snd_pcm_uframes_t Source::copyData(unsigned char* srcBuffer, unsigned char* dstBuffer, snd_pcm_uframes_t frames)
		{
			auto bytesPerFrame{parameters.getTotalChannels() * (parameters.getBitsPerSample() >> 3)};
			auto framesCopied{snd_pcm_sframes_t{0}};

			for (snd_pcm_uframes_t i = 0; i < frames; i++)
			{
				StreamMarker value{srcBuffer[(i + 1) * bytesPerFrame - 1]};  // last byte of the current frame
				if (value == StreamMarker::beginningOfStream)
				{
					streaming = true;
				}
				else if (value == StreamMarker::endOfStream)
				{
					streaming = false;
				}
				else if (value == StreamMarker::data && streaming)
				{
					// copying byte-by-byte and skipping the last channel
					for (unsigned int j = 0; j < (bytesPerFrame - (parameters.getBitsPerSample() >> 3)); j++)
					{
						dstBuffer[j] = srcBuffer[i * bytesPerFrame + j];
					}

					// adjusting destination buffer pointer
					dstBuffer += parameters.getLogicalChannels() * (parameters.getBitsPerSample() >> 3);

					// increasing destination frames counter
					framesCopied++;
				}
			}

			// returning copied size in frames
			return framesCopied;
		}


		void Source::open()
		{
			snd_pcm_hw_params_t* hardwarePtr  = nullptr;
			snd_pcm_sw_params_t* softwarePtr  = nullptr;
			auto                 deviceName   = parameters.getDeviceName();
			auto                 samplingRate = parameters.getSamplingRate();
			int                  result;
			::util::scope_guard  guard        = [&]
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
			};

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
			else if ((result = snd_pcm_hw_params_set_rate_near(handlePtr, hardwarePtr, &samplingRate, nullptr)) < 0)
			{
				throw Exception(formatError("Cannot set sample rate", result));
			}
			else if ((result = snd_pcm_hw_params_set_channels(handlePtr, hardwarePtr, parameters.getTotalChannels())) < 0)
			{
				throw Exception(formatError("Cannot set channel count", result));
			}
			else if ((result = snd_pcm_hw_params_set_period_size(handlePtr, hardwarePtr, parameters.getFramesPerChunk(), 0)) < 0)
			{
				throw Exception(formatError("Cannot set period size", result));
			}
			else if ((result = snd_pcm_hw_params_set_periods(handlePtr, hardwarePtr, parameters.getPeriods(), 0)) < 0)
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

			// start threshold is irrelevant for capturing PCM so setting it to 1
			else if ((result = snd_pcm_sw_params_set_start_threshold(handlePtr, softwarePtr, 1U)) < 0)
			{
				throw Exception(formatError("Cannot set start threshold", result));
			}
			else if ((result = snd_pcm_sw_params(handlePtr, softwarePtr)) < 0)
			{
				throw Exception(formatError("Cannot set software parameters", result));
			}

			// start receiving data from ALSA
			else if ((result = snd_pcm_start(handlePtr)) < 0)
			{
				throw Exception(formatError("Cannot start using audio device", result));
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


		void Source::start()
		{
			auto          maxFrames     = parameters.getFramesPerChunk();
			unsigned int  bytesPerFrame = parameters.getTotalChannels() * (parameters.getBitsPerSample() >> 3);
			unsigned char srcBuffer[maxFrames * bytesPerFrame];

			// changing state to 'running' in a thread-safe way
			{
				std::lock_guard<std::mutex> lockGuard{lock};
				if (running)
				{
					throw Exception(formatError("Audio device was already started"));
				}

				// open ALSA device
				open();
				running = true;
			}

			// everything inside this loop (except overflowCallback) must be real-time safe: no memory allocation, no logging, etc.
			snd_pcm_sframes_t result{0};
			while (result >= 0)
			{
				// this call will block until buffer is filled or PCM stream state is changed
				result = snd_pcm_readi(handlePtr, srcBuffer, maxFrames);

				// if PCM data is available if the buffer
				if (result > 0)
				{
					auto offset = containsData(srcBuffer, static_cast<snd_pcm_uframes_t>(result));

					// if PCM data contains active stream and its offset within the range
					if (offset >= 0 && offset < result)
					{
						// changing state to 'producing'
						producing = true;

						// enqueue received PCM data so that none-Real-Time safe code can process it
						queuePtr->enqueue([&](Chunk& chunk)
						{
							// setting chunk 'meta' data
							chunk.setSamplingRate(parameters.getSamplingRate());
							chunk.setChannels(parameters.getLogicalChannels());
							chunk.setBitsPerSample(parameters.getBitsPerSample());
							chunk.setEndOfStream(false);

							// TODO: move functionality to Chunk class
							// copying data and setting new chunk size in bytes
							chunk.setSize(copyData(srcBuffer + offset * bytesPerFrame, chunk.getData(), static_cast<snd_pcm_uframes_t>(result - offset)) * parameters.getLogicalChannels() * (parameters.getBitsPerSample() >> 3));

							// keep track on amount of processed chunks
							chunkCounter++;

							// always true as source buffer contains data
							return true;
						}, [&]
						{
							// calling overflow callback in case it was not possible to enqueue a chunk
							overflowCallback();
						});
					}
					else
					{
						// changing state to 'not producing'
						if (producing)
						{
							// creating an 'empty' chunk required to mark end-of-stream
							queuePtr->enqueue([&](Chunk& chunk)
							{
								chunk.setSamplingRate(parameters.getSamplingRate());
								chunk.setChannels(parameters.getLogicalChannels());
								chunk.setBitsPerSample(parameters.getBitsPerSample());
								chunk.setEndOfStream(true);
								chunk.setSize(0);

								// keep track on amount of processed chunks
								chunkCounter++;

								// always true as source buffer contains data
								return true;
							}, [&]
							{
								// calling overflow callback in case it was not possible to enqueue a chunk
								overflowCallback();
							});
						}
						producing    = false;
						chunkCounter = 0;
					}
				}
				else if (result < 0 && restore(result))
				{
					// error was recovered so keep processing
					result = 0;
				}
			}

			// if error code is unexpected then breaking this loop (-EBADFD is returned when stop method is called)
			if (result != -EBADFD)
			{
				LOG(ERROR) << LABELS{"alsa"} << formatError("Unexpected error while reading PCM data", result);
			}

			// changing state to 'not running' in a thread-safe way
			{
				std::lock_guard<std::mutex> lockGuard{lock};
				running = false;

				// closing ALSA device
				close();
			}
		}


		void Source::stop(bool gracefully)
		{
			// issuing a request to stop receiving PCM data
			{
				std::lock_guard<std::mutex> lockGuard{lock};
				if (running)
				{
					int result;
					if (gracefully)
					{
						if ((result = snd_pcm_drain(handlePtr)) < 0)
						{
							LOG(ERROR) << LABELS{"alsa"} << formatError("Error while stopping PCM stream gracefully", result);
						}
					}
					else
					{
						if ((result = snd_pcm_drop(handlePtr)) < 0)
						{
							LOG(ERROR) << LABELS{"alsa"} << formatError("Error while stopping PCM stream unconditionally", result);
						}
					}
				}
			}

			// waiting for this ALSA device to stop receiving PCM data
			while (isRunning())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds{10});
			}
		}
	}
}
