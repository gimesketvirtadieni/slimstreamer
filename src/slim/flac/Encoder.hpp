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
#include <type_safe/reference.hpp>

#include "slim/log/log.hpp"
#include "slim/util/BufferedWriter.hpp"
#include "slim/util/Writer.hpp"


namespace slim
{
	namespace flac
	{
		class Encoder : protected FLAC::Encoder::Stream
		{
			public:
				explicit Encoder(unsigned int c, unsigned int s, unsigned int b, type_safe::object_ref<util::Writer> w, bool h)
				: channels{c}
				, sampleRate{s}
				, bitsPerSample{b}
				, bufferedWriter{w}
				, bytesPerFrame{channels * (bitsPerSample >> 3)}
				, byteRate{sampleRate * bytesPerFrame}
				{
					auto ok{true};

					// do not validate whether the stream is bit-perfect to the original PCM data
					ok &= set_verify(false);
					ok &= set_compression_level(8);
					ok &= set_channels(channels);
					ok &= set_sample_rate(sampleRate);
					// TODO: FLAC does not support 32 bits per sample
					ok &= set_bits_per_sample(24);

					// choosing big enough number of expected samples for streaming purpose
					ok &= set_total_samples_estimate(0xFFFFFFFF);

					if (ok)
					{
						auto init_status{init()};
						if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
						{
							LOG(ERROR) << LABELS{"flac"} << FLAC__StreamEncoderInitStatusString[init_status];
							ok = false;
						}
					}

					if (!ok)
					{
						LOG(ERROR) << LABELS{"flac"} << "Encoder initialization error";
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
					// TODO: introduce buffer available method
					//if (getFreeBufferIndex().has_value())
					{
						auto samples{size >> 2};
						auto frames{samples >> 1};

						// checking if only 24 bits are used for PCM data
						auto scalingWarning{false};
						for (std::size_t i = 0; i < size; i += 4)
						{
							if (data[i])
							{
								// TODO: consider other place for this scaling to 24 bits
								data[i] = 0;
								if (!scalingWarning)
								{
									LOG(WARNING) << LABELS{"flac"} << "All 32-bits are used for PCM data, scaling to 24 bits as required for FLAC";
									scalingWarning = true;
								}
							}
						}

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
								data[samples * 4 - 7],
								data[samples * 4 - 6],
								data[samples * 4 - 5],
								0,
								data[samples * 4 - 3],
								data[samples * 4 - 2],
								data[samples * 4 - 1],
								0
							};

							if (!process_interleaved((const FLAC__int32*)lastFrame, 1))
							{
								LOG(ERROR) << LABELS{"flac"} << get_state().as_cstring();
							}
						}

						// is used to notify the caller about amount of data processed
						encoded = size;
					}
					//else
					//{
					//	LOG(WARNING) << LABELS{"flac"} << "Transfer buffer is full - skipping PCM chunk";
					//}

					return encoded;
				}

				auto getMIME()
				{
					return std::string{"audio/flac"};
				}

			protected:
				virtual ::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte* data, std::size_t size, unsigned samples, unsigned current_frame) override
				{
					// TODO: handle errors properly
					bufferedWriter.writeAsync(data, size);
					//else
					//{
					//	LOG(WARNING) << LABELS{"flac"} << "Transfer buffer is full - skipping encoded chunk";
					//}

					return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
				}

			private:
				unsigned int             channels;
				unsigned int             sampleRate;
				unsigned int             bitsPerSample;
				// TODO: parametrize
				util::BufferedWriter<10> bufferedWriter;
				unsigned int             bytesPerFrame;
				unsigned int             byteRate;
		};
	}
}
