#ifdef USE_FLAC

#include "shim4/main.h"
#include "shim4/util.h"

#define FLAC__NO_DLL
#include "FLAC/stream_decoder.h"

using namespace noo;

namespace noo {

namespace audio {

struct My_FLAC_Info
{
	SDL_RWops *file;
	SDL_AudioSpec *spec;
	Uint8 *buf;
	int p;
};

static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
static void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

Uint8 *decode_flac(SDL_RWops *file, char *errmsg, SDL_AudioSpec *spec)
{
	FLAC__bool ok = true;
	FLAC__StreamDecoder *decoder = 0;
	FLAC__StreamDecoderInitStatus init_status;

	if((decoder = FLAC__stream_decoder_new()) == NULL) {
		strcpy(errmsg, "Error allocating FLAC decoder.\n");
		return nullptr;
	}

	(void)FLAC__stream_decoder_set_md5_checking(decoder, true);

	My_FLAC_Info *info = new My_FLAC_Info;
	info->file = file;
	info->spec = spec;
	info->buf = nullptr;
	info->p = 0;

	init_status = FLAC__stream_decoder_init_stream(decoder, read_callback, nullptr, nullptr, nullptr, nullptr, write_callback, metadata_callback, error_callback, info);
	if(init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		sprintf(errmsg, "Error initializing FLAC decoder: %s.\n", FLAC__StreamDecoderInitStatusString[init_status]);
		ok = false;
	}

	if(ok) {
		ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
		if (ok == false) {
			sprintf(errmsg, "FLAC decoding failed: %s.\n", FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);
		}
	}

	FLAC__stream_decoder_delete(decoder);

	Uint8 *ret = info->buf;

	delete info;

	if (ok == false) {
		delete ret;
		return nullptr;
	}

	return ret;
}

FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	My_FLAC_Info *info = static_cast<My_FLAC_Info *>(client_data);
	size_t i;

	(void)decoder;

	//util::debugmsg("Write %u\n", frame->header.blocksize);

	/* write decoded PCM samples */
	if (info->spec->channels == 1) {
		if (info->spec->format == AUDIO_S16) {
			for(i = 0; i < frame->header.blocksize; i++) {
				FLAC__int16 mono = buffer[0][i];
				info->buf[info->p++] = mono & 0xff;
				info->buf[info->p++] = (mono >> 8) & 0xff;
			}
		}
	}
	else {
		if (info->spec->format == AUDIO_S16) {
			for(i = 0; i < frame->header.blocksize; i++) {
				FLAC__int16 left = buffer[0][i];
				FLAC__int16 right = buffer[1][i];
				info->buf[info->p++] = left & 0xff;
				info->buf[info->p++] = (left >> 8) & 0xff;
				info->buf[info->p++] = right & 0xff;
				info->buf[info->p++] = (right >> 8) & 0xff;
			}
		}
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	My_FLAC_Info *info = static_cast<My_FLAC_Info *>(client_data);

	/* print some stats */
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		info->spec->freq = metadata->data.stream_info.sample_rate;
		info->spec->channels = metadata->data.stream_info.channels;
		info->spec->size = int(metadata->data.stream_info.total_samples * (metadata->data.stream_info.bits_per_sample/8) * info->spec->channels);
		info->spec->format = metadata->data.stream_info.bits_per_sample == 16 ? AUDIO_S16 : 0;
		info->spec->silence = 0;
		info->spec->samples = 4096;
		info->spec->callback = 0;
		info->spec->userdata = 0;
		info->buf = new Uint8[info->spec->size];
		//util::debugmsg("FLAC freq=%d\n", info->spec->freq);
		//util::debugmsg("FLAC channels=%d\n", info->spec->channels);
		//util::debugmsg("FLAC size=%d\n", info->spec->size);
		//util::debugmsg("FLAC format=%d (S16=%d)\n", info->spec->format, AUDIO_S16);
	}
}

FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	My_FLAC_Info *info = static_cast<My_FLAC_Info *>(client_data);

	*bytes = SDL_RWread(info->file, buffer, sizeof(FLAC__byte), *bytes);

	if (*bytes == 0) {
	       return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}

	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	(void)decoder, (void)client_data;

	util::debugmsg("Got FLAC error: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

} // End namespace audio

} // End namespace noo

#endif // USE_FLAC
