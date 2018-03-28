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

#include "slim/log/log.hpp"
#include "slim/StreamWriter.hpp"
#include "slim/util/ExpandableBuffer.hpp"


namespace slim
{
	namespace flac
	{
		class Stream : public StreamWriter, public FLAC::Encoder::Stream
		{
			public:
				explicit Stream(StreamWriter* w, unsigned int c, unsigned int s, unsigned int b)
				: StreamWriter
				{
					std::move([&](auto* data, auto size) mutable
					{
						return encode(data, size);
					})
				}
				, writerPtr{w}
				, channels{c}
				, sampleRate{s}
				, bitsPerSample{b}
				, bytesPerFrame{channels * (bitsPerSample >> 3)}
				, byteRate{sampleRate * bytesPerFrame}
				{
					auto ok{true};

					// TODO: encoding fails with FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA if verity is set to true
					ok &= set_verify(false);
					ok &= set_compression_level(5);
					ok &= set_channels(channels);
					ok &= set_sample_rate(sampleRate);
					// TODO: FLAC does not support 32 bits per sample
					ok &= set_bits_per_sample(24);
					//ok &= set_total_samples_estimate(total_samples);
/*
					// now add some metadata; we'll add some tags and a padding block
					if(ok) {
						if(
							(metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL ||
							(metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL ||
							// there are many tag (vorbiscomment) functions but these are convenient for this particular use:
							!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", "Some Artist") ||
							!FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false) || // copy=false: let metadata object take control of entry's allocated string
							!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "YEAR", "1984") ||
							!FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false)
						) {
							fprintf(stderr, "ERROR: out of memory or tag error\n");
							ok = false;
						}

						metadata[1]->length = 1234; // set the padding length

						ok = encoder.set_metadata(metadata, 2);
					}
*/
					if(ok)
					{
						auto init_status{init()};
						if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
						{
							LOG(ERROR) << LABELS{"flac"} << FLAC__StreamEncoderInitStatusString[init_status];
							ok = false;
						}
					}

					if(!ok)
					{
						LOG(ERROR) << LABELS{"flac"} << "Initialization error";
					}
				}

				virtual ~Stream()
				{
					if (!finish())
					{
						LOG(INFO) << LABELS{"flac"} << "Finish failed:" << get_state().as_cstring();
					}

					// now that encoding is finished, the metadata can be freed
					//FLAC__metadata_object_delete(metadata[0]);
					//FLAC__metadata_object_delete(metadata[1]);
				}

				Stream(const Stream&) = delete;             // non-copyable
				Stream& operator=(const Stream&) = delete;  // non-assignable
				Stream(Stream&&) = delete;                  // non-movable
				Stream& operator=(Stream&&) = delete;       // non-assign-movable

				virtual std::string getMIME()
				{
					return "audio/flac";
				}

				virtual void rewind(const std::streampos pos)
				{
					writerPtr->rewind(pos);
				}

				virtual void writeAsync(const void* data, const std::size_t size, WriteCallback callback) override
				{
					// TODO: temporary solution, implement async transfer
					write(data, size);
					callback(std::error_code(), size);
				}

			protected:
				std::streamsize encode(const char* data, const std::streamsize size)
				{
					// allocating buffer
					// TODO: avoid type conversion
					if ((std::streamsize)buffer.capacity() < size)
					{
						buffer.capacity(size);
					}

					auto samples{size >> 2};
					auto frames{samples >> 1};
					auto outputData{buffer.data()};
					for (auto i{0}; i < samples; i++)
					{
						if (data[i * 4])
						{
							LOG(WARNING) << LABELS{"flac"} << "PCM data uses 32 bits, this is not supported by FLAC";
						}

						// TODO: implement bulk copy
						outputData[i * 4]     = data[i * 4 + 1];
						outputData[i * 4 + 1] = data[i * 4 + 2];
						outputData[i * 4 + 2] = data[i * 4 + 3];
						outputData[i * 4 + 3] = 0;
					}

					if(!process_interleaved((const FLAC__int32*)outputData, frames))
					{
						LOG(ERROR) << LABELS{"flac"} << get_state().as_cstring();
					}

					return size;
				}

				virtual ::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], std::size_t bytes, unsigned samples, unsigned current_frame) override
				{
					// TODO: handle exceptions
					writerPtr->write(buffer, bytes);

					return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
				}

			private:
				StreamWriter*          writerPtr;
				unsigned int           channels;
				unsigned int           sampleRate;
				unsigned int           bitsPerSample;
				unsigned int           bytesPerFrame;
				unsigned int           byteRate;
				util::ExpandableBuffer buffer;
		};
	}
}
