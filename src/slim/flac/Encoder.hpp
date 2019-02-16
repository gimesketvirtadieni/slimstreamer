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

#include <cstddef>   // std::size_t
#include <FLAC++/encoder.h>
#include <string>

#include "slim/EncoderBase.hpp"
#include "slim/Exception.hpp"
#include "slim/log/log.hpp"


namespace slim
{
	namespace flac
	{
		class Encoder : public EncoderBase, protected FLAC::Encoder::Stream
		{
			public:
				explicit Encoder(unsigned int ch, unsigned int bs, unsigned int bv, unsigned int sr, bool hd, std::string ex, std::string mm, EncodedCallbackType ec)
				: EncoderBase{ch, bs, bv, sr, ex, mm, ec} {}

				virtual ~Encoder() = default;
				Encoder(const Encoder&) = delete;             // non-copyable
				Encoder& operator=(const Encoder&) = delete;  // non-assignable
				Encoder(Encoder&&) = default;
				Encoder& operator=(Encoder&&) = default;

				virtual void encode(unsigned char* data, const std::size_t size) override
				{
					if (running)
					{
						std::size_t sampleSize{getBitsPerSample() >> 3};
						std::size_t frameSize{sampleSize * getChannels()};
						std::size_t frames{size / frameSize};

						// if values contain more than 24 bits then downscaling to 24 bits, which is max supported by FLAC
						if (downScale)
						{
							for (std::size_t i = 0; i < size; i += sampleSize)
							{
								data[i] = 0;
							}
						}

						// coverting data S32_LE to S24_LE by shifting data by 1 byte
						if (frames > 1 && !process_interleaved((const FLAC__int32*)(data + 1), frames - 1))
						{
							LOG(ERROR) << LABELS{"flac"} << "Error while encoding: " << get_state().as_cstring();
						}

						// handling the last frame separately; shifting the last frame data by one byte
						if (frames > 0)
						{
							unsigned char  frame[frameSize];
							unsigned char* lastFrame{data + size - frameSize};

							for (auto i{frameSize - 1}; i > 0; i--)
							{
								frame[i - 1] = lastFrame[i];
							}
							frame[frameSize - 1] = 0;

							if (!process_interleaved((const FLAC__int32*)frame, 1))
							{
								LOG(ERROR) << LABELS{"flac"} << "Error while encoding: " << get_state().as_cstring();
							}
						}
					}
				}

				virtual bool isRunning() override
				{
					return running;
				}

				virtual void start() override
				{
					// initializing encoder only if it is not initialized
					if (!running)
					{
						// do not validate FLAC encoded stream if it produces the same result
						if (!set_verify(false))
						{
							throw Exception("Could not disable FLAC stream verification");
						}

						// setting minimal compression level (using max level does not gain much)
						if (!set_compression_level(0))
						{
							throw Exception("Could not set compression level");
						}

						// making sure encoded output is suitable for streaming
						if (!set_streamable_subset(true))
						{
							throw Exception("Could not select streamable subset");
						}

						// setting amount of channels
						if (!set_channels(getChannels()))
						{
							throw Exception("Could not set amount of channels");
						}

						// setting sampling rate
						if (!set_sample_rate(getSamplingRate()))
						{
							throw Exception("Could not set sampling rate");
						}

						// only 32 bit per sample for input PCM is supported
						if (getBitsPerSample() != 32)
						{
							throw Exception("Format with 32 bits per sample is only supported");
						}

						// FLAC encoding support max 24 bits per value
						auto b{getBitsPerValue()};
						if (b > 24)
						{
							LOG(WARNING) << LABELS{"flac"} << "PCM data will be scaled to 24 bits values, which is max bit depth supported by FLAC";

							b         = 24;
							downScale = true;
						}

						// setting sampling rate
						if (!set_bits_per_sample(b))
						{
							throw Exception("Could not set bits per sample");
						}

						// initializing FLAC encoder
						if (auto init_status{init()}; init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
						{
							throw Exception(FLAC__StreamEncoderInitStatusString[init_status]);
						}

						running = true;
					}
				}

				virtual void stop(std::function<void()> callback) override
				{
					if (running)
					{
						if (!finish())
						{
							LOG(ERROR) << LABELS{"flac"} << "Error while stopping encoder: " << get_state().as_cstring();
						}

						running = false;
					}

					// notifying that object can be deleted
					callback();
				}

			protected:
				virtual ::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte* data, std::size_t size, unsigned samples, unsigned current_frame) override
				{
					getEncodedCallback()((unsigned char*)data, size);

					return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
				}

			private:
				bool running{false};
				bool downScale{false};
		};
	}
}
