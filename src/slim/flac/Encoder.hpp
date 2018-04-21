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

#include "slim/log/log.hpp"
#include "slim/util/AsyncWriter.hpp"
#include "slim/util/BufferedAsyncWriter.hpp"


namespace slim
{
	namespace flac
	{
		class Encoder : protected FLAC::Encoder::Stream
		{
			public:
				explicit Encoder(unsigned int c, unsigned int s, unsigned int bs, unsigned int bv, std::reference_wrapper<util::AsyncWriter> w, bool h)
				: channels{c}
				, sampleRate{s}
				, bitsPerSample{bs}
				, bitsPerValue{bv}
				, bufferedWriter{w}
				{
					// do not validate FLAC encoded stream if it produces the same result
					if (!set_verify(false))
					{
						throw Exception("Could not disable FLAC stream verification");
					}

					// setting maximum possible compression level
					if (!set_compression_level(8))
					{
						throw Exception("Could not set compression level");
					}

					// setting amount of channels
					if (!set_channels(channels))
					{
						throw Exception("Could not set amount of channels");
					}

					// setting sampling rate
					if (!set_sample_rate(sampleRate))
					{
						throw Exception("Could not set sampling rate");
					}

					// FLAC encoding support max 24 bits per value
					auto b{bitsPerValue};
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

					// choosing big enough number of expected samples for streaming purpose
					if (!set_total_samples_estimate(0xFFFFFFFF))
					{
						throw Exception("Could not set estimated amount of samples");
					}

					// initializing FLAC encoder
					auto init_status{init()};
					if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
					{
						throw Exception(FLAC__StreamEncoderInitStatusString[init_status]);
					}
				}

			   ~Encoder()
				{
					if (!finish())
					{
						LOG(ERROR) << LABELS{"flac"} << "Error while closing encoder: " << get_state().as_cstring();
					}
				}

				Encoder(const Encoder&) = delete;             // non-copyable
				Encoder& operator=(const Encoder&) = delete;  // non-assignable
				Encoder(Encoder&&) = delete;                  // non-movable
				Encoder& operator=(Encoder&&) = delete;       // non-assign-movable

				auto encode(unsigned char* data, const std::size_t size)
				{
					std::size_t encoded{0};

					// do not feed encoder with more data if there is no room in transfer buffer
					if (bufferedWriter.isBufferAvailable())
					{
						std::size_t bytesPerSample{bitsPerSample >> 3};
						std::size_t samples{size / bytesPerSample};
						std::size_t frames{samples / channels};

						// if values contain more than 24 bits then downscaling to 24 bits, which is max supported by FLAC
						if (downScale)
						{
							for (std::size_t i = 0; i < size; i += bytesPerSample)
							{
								data[i] = 0;
							}
						}

						// TODO: generialize based on parameters (bitsPerFrame and bitsPerValue)
						// coverting data S32_LE to S24_LE by shifting data by 1 byte
						if (frames > 1 && !process_interleaved((const FLAC__int32*)(data + 1), frames - 1))
						{
							LOG(ERROR) << LABELS{"flac"} << get_state().as_cstring();
						}

						// handling the last frame separately; requred due to data shift
						if (frames > 0)
						{
							unsigned char lastFrame[8] =
							{
								data[size - 7],
								data[size - 6],
								data[size - 5],
								0,
								data[size - 3],
								data[size - 2],
								data[size - 1],
								0
							};

							if (!process_interleaved((const FLAC__int32*)lastFrame, 1))
							{
								LOG(ERROR) << LABELS{"flac"} << get_state().as_cstring();
							}
						}

						// notifing the caller about amount of data processed
						encoded = size;
					}
					else
					{
						LOG(WARNING) << LABELS{"flac"} << "Transfer buffer is full - skipping PCM chunk";
					}

					return encoded;
				}

				auto getMIME()
				{
					return std::string{"audio/flac"};
				}

			protected:
				virtual ::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte* data, std::size_t size, unsigned samples, unsigned current_frame) override
				{
					bufferedWriter.writeAsync(data, size, [](auto error, auto written)
					{
						if (error)
						{
							LOG(ERROR) << LABELS{"flac"} << "Error while encoded data transfer: " << error.message();
						}
					});

					return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
				}

			private:
				unsigned int                  channels;
				unsigned int                  sampleRate;
				unsigned int                  bitsPerSample;
				unsigned int                  bitsPerValue;
				bool                          downScale{false};
				// TODO: parametrize
				util::BufferedAsyncWriter<10> bufferedWriter;
		};
	}
}
