#ifdef USE_BASS
#include <bass.h>
#endif

#include "shim4/error.h"
#include "shim4/flac.h"
#include "shim4/sample.h"
#include "shim4/shim.h"
#include "shim4/util.h"
#include "shim4/vorbis.h"

#include "shim4/internal/audio.h"

using namespace noo;

namespace noo {

namespace audio {

void Sample::stop_instance(Sample_Instance *s)
{
	SDL_LockMutex(internal::audio_context.mixer_mutex);

	for (std::vector<Sample_Instance *>::iterator it = internal::audio_context.playing_samples.begin(); it != internal::audio_context.playing_samples.end(); it++) {
		Sample_Instance *s2 = *it;
		if (s2 == s) {
			internal::audio_context.playing_samples.erase(it);
			delete s;
			break;
		}
	}

	SDL_UnlockMutex(internal::audio_context.mixer_mutex);
}

Sample::Sample(std::string filename, bool load_from_filesystem) :
	done(false)
{
	if (internal::audio_context.mute) {
		spec = NULL;
		file = NULL;
		data = NULL;
		return;
	}

#if defined USE_BASS || defined USE_VORBIS
	if (filename.find(".ogg") != std::string::npos) {
#ifdef USE_BASS
		int sz;
		Uint8 *tmpdata;

		if (load_from_filesystem) {
			throw util::Error("Loading OGG from filesystem not supported yet!");
		}
		else {
			filename = "audio/samples/" + filename;
			tmpdata = (Uint8 *)util::open_file(filename, &sz, true);
		}
		
		file = 0;

		HSTREAM s;
		if ((s = BASS_StreamCreateFile(TRUE, tmpdata, 0, sz, BASS_STREAM_DECODE | BASS_SAMPLE_MONO)) == 0) {
			delete[] tmpdata;
			throw util::LoadError("BASS_SampleLoad failed (" + util::itos(BASS_ErrorGetCode()) + ")!");
		}

		BASS_ChannelSetAttribute(s, BASS_ATTRIB_VOL, 1.0f);

		const int chunk_size = 4096;

		length = 0;
		DWORD to_read = BASS_ChannelGetLength(s, BASS_POS_BYTE);
		if (to_read == (DWORD)-1) {
			to_read = chunk_size;
		}
		else {
			int n = to_read / chunk_size;
			to_read = (n+1) * chunk_size; // make a multiple of chunk_size, SDL needs it this way (see spec->samples below)
		}
		data = new Uint8[to_read];

		while (true) {
			DWORD r;
			if ((r = BASS_ChannelGetData(s, data+length, to_read)) < (DWORD)to_read || r == (DWORD)-1) {
				DWORD error;
				if (r != (DWORD)-1) {
					length += r;
				}
				else if ((error = BASS_ErrorGetCode()) != BASS_ERROR_ENDED) {
					throw util::Error("Error decoding OGG (" + util::itos(error) + ")!");
				}
				break;
			}
			length += r;
			Uint8 *newdata = new Uint8[length+chunk_size];
			memcpy(newdata, data, length);
			delete[] data;
			data = newdata;
			to_read = chunk_size;
		}

		BASS_CHANNELINFO info;

		if (BASS_ChannelGetInfo(s, &info) == FALSE) {
			BASS_StreamFree(s);
			delete[] tmpdata;
			delete[] data;
			throw util::Error("BASS_ChannelGetInfo failed (" + util::itos(BASS_ErrorGetCode()) + ")!");
		}

		spec = new SDL_AudioSpec;

		spec->freq = info.freq;
		spec->format = AUDIO_S16;
		spec->channels = info.chans;
		spec->silence = 0;
		spec->samples = chunk_size; // SDL docs say when SDL_LoadWav is used, this is set to 4096
		spec->size = length;
		spec->callback = 0;
		spec->userdata = 0;

		BASS_StreamFree(s);

		delete[] tmpdata;
#else
		// We can't close the file yet... forget why (I think FreeWAV closes it)
		if (load_from_filesystem) {
			file = SDL_RWFromFile(filename.c_str(), "r");
		}
		else {
			filename = "audio/samples/" + filename;
			file = util::open_file(filename, 0);
		}

		char errmsg[1000];

		spec = new SDL_AudioSpec;

		data = audio::decode_vorbis(file, errmsg, spec);
		
		if (load_from_filesystem) {
			SDL_RWclose(file);
		}
		else {
			util::close_file(file);
		}

		if (data == 0) {
			delete spec;
			throw util::Error(errmsg);
		}

		length = spec->size / spec->channels / (SDL_AUDIO_BITSIZE(spec->format)/8);

		file = 0;
#endif
	}
	else
#endif
#if defined USE_FLAC
	if (filename.find(".flac") != std::string::npos) {
		// We can't close the file yet... forget why (I think FreeWAV closes it)
		if (load_from_filesystem) {
			file = SDL_RWFromFile(filename.c_str(), "r");
		}
		else {
			filename = "audio/samples/" + filename;
			file = util::open_file(filename, 0);
		}

		char errmsg[1000];

		spec = new SDL_AudioSpec;

		data = audio::decode_flac(file, errmsg, spec);
		
		if (load_from_filesystem) {
			SDL_RWclose(file);
		}
		else {
			util::close_file(file);
		}

		if (data == 0) {
			delete spec;
			util::debugmsg(errmsg);
			throw util::Error(errmsg);
		}

		length = spec->size / spec->channels / (SDL_AUDIO_BITSIZE(spec->format)/8);

		file = 0;
	}
	else
#endif
	{
		// We can't close the file yet... forget why (I think FreeWAV closes it)
		if (load_from_filesystem) {
			file = SDL_RWFromFile(filename.c_str(), "r");
		}
		else {
			filename = "audio/samples/" + filename;
			file = util::open_file(filename, 0);
		}

		spec = new SDL_AudioSpec;

		if (SDL_LoadWAV_RW(file, false, spec, &data, &length) == 0) {
			util::close_file(file);
			throw util::LoadError("SDL_LoadWAV_RW failed");
		}

		int orig_len = length;
		length = length / spec->channels / (SDL_AUDIO_BITSIZE(spec->format)/8);

		SDL_AudioCVT cvt;
		SDL_AudioFormat out_format;
		if (util::bool_arg(false, shim::argc, shim::argv, "16bit-samples")) {
			out_format = AUDIO_S16;
		}
		else {
			out_format = AUDIO_F32;
		}

		if (SDL_BuildAudioCVT(&cvt, spec->format, spec->channels, spec->freq, out_format, 2, internal::audio_context.device_spec.freq) == 1) {
			SDL_assert(cvt.needed);
			cvt.len = orig_len;
			cvt.buf = new Uint8[cvt.len * cvt.len_mult];
			memcpy(cvt.buf, data, orig_len);
			SDL_ConvertAudio(&cvt);

			SDL_FreeWAV(data);
			util::close_file(file);
			file = nullptr;

			data = cvt.buf;

			spec->format = out_format;
			spec->channels = 2;
			spec->freq = internal::audio_context.device_spec.freq;
			spec->size = cvt.len_cvt;
		
			length = spec->size / spec->channels / (SDL_AUDIO_BITSIZE(spec->format)/8);
		}
	}
}

void Sample::delete_instances()
{
	std::vector<Sample_Instance *>::iterator it;
	for (it = internal::audio_context.playing_samples.begin(); it != internal::audio_context.playing_samples.end();) {
		Sample_Instance *s = *it;
		if (s->spec == spec) {
			it = internal::audio_context.playing_samples.erase(it);
			delete s;
		}
		else {
			it++;
		}
	}
}

Sample::~Sample()
{
	delete_instances();
	if (file) {
		SDL_FreeWAV(data);
		util::close_file(file);
	}
	else {
		delete[] data;
	}

	delete spec;
}

void Sample::play(bool loop)
{
	play(1.0f, loop);
}

// FIXME: avoid repetition here with other play method
void Sample::play(float volume, bool loop, int type)
{
	if (internal::audio_context.mute) {
		return;
	}

	Sample_Instance *s = new Sample_Instance();
	if (s == 0) {
		return;
	}

	done = false;

	float p = (float)internal::audio_context.device_spec.freq / spec->freq;

	s->spec = spec;
	s->data = data;
	s->length = length;
	s->play_length = length * p;
	s->offset = 0;
	s->silence = 0;
	s->loop = loop;
	s->volume = volume;
	s->sample = this;
	s->bits_per_sample = SDL_AUDIO_BITSIZE(spec->format);
	s->bytes_per_sample = s->bits_per_sample / 8;
	s->format_is_float = SDL_AUDIO_ISFLOAT(spec->format);
	s->format_is_signed = SDL_AUDIO_ISSIGNED(spec->format);
	s->format_should_be_swapped = (SDL_AUDIO_ISBIGENDIAN(spec->format) && SDL_BYTEORDER == SDL_LIL_ENDIAN) || (SDL_AUDIO_ISLITTLEENDIAN(spec->format) && SDL_BYTEORDER == SDL_BIG_ENDIAN);
	if (s->format_is_float) {
		s->min_sample = -1.0f;
		s->max_sample = 1.0f;
	}
	else {
		s->min_sample = -powf(2, s->bits_per_sample-1);
		s->max_sample = powf(2, s->bits_per_sample-1) - 1;
	}

	s->type = type;
	s->master_volume = 1.0f;
	s->channels = spec->channels;

	SDL_LockMutex(internal::audio_context.mixer_mutex);
	delete_instances();
	internal::audio_context.playing_samples.push_back(s);
	SDL_UnlockMutex(internal::audio_context.mixer_mutex);
}

void Sample::play(float volume, bool loop)
{
	play(volume, loop, 0);
}

Sample_Instance *Sample::play_stretched(float volume, Uint32 silence, Uint32 play_length, int type)
{
	if (internal::audio_context.mute) {
		return 0;
	}

	Sample_Instance *s = new Sample_Instance();
	if (s == 0) {
		return 0;
	}

	done = false;

	int bits_per_sample = SDL_AUDIO_BITSIZE(spec->format);
	int bytes_per_sample = bits_per_sample / 8;
	float p = (float)internal::audio_context.device_spec.freq / spec->freq;

	s->spec = spec;
	s->data = data;
	s->length = length;
	s->play_length = (play_length == 0 ? length * p : play_length);
	s->offset = 0;
	s->silence = silence;
	s->loop = false;
	s->volume = volume;
	s->sample = this;
	s->bits_per_sample = bits_per_sample;
	s->bytes_per_sample = bytes_per_sample;
	s->format_is_float = SDL_AUDIO_ISFLOAT(spec->format);
	s->format_is_signed = SDL_AUDIO_ISSIGNED(spec->format);
	s->format_should_be_swapped = (SDL_AUDIO_ISBIGENDIAN(spec->format) && SDL_BYTEORDER == SDL_LIL_ENDIAN) || (SDL_AUDIO_ISLITTLEENDIAN(spec->format) && SDL_BYTEORDER == SDL_BIG_ENDIAN);
	if (s->format_is_float) {
		s->min_sample = -1.0f;
		s->max_sample = 1.0f;
	}
	else {
		s->min_sample = -powf(2, s->bits_per_sample-1);
		s->max_sample = powf(2, s->bits_per_sample-1) - 1;
	}

	s->type = type;
	s->master_volume = 1.0f;
	s->channels = spec->channels;

	SDL_LockMutex(internal::audio_context.mixer_mutex);
	internal::audio_context.playing_samples.push_back(s);
	SDL_UnlockMutex(internal::audio_context.mixer_mutex);

	return s;
}

bool Sample::is_done()
{
	return done;
}

void Sample::set_done(bool done)
{
	this->done = done;
}

// We don't return sample instances to the user (we could) so stop has to stop all instances of this sample
void Sample::stop_all()
{
	SDL_LockMutex(internal::audio_context.mixer_mutex);

	std::vector<Sample_Instance *>::iterator it;

	for (it = internal::audio_context.playing_samples.begin(); it != internal::audio_context.playing_samples.end();) {
		Sample_Instance *s = *it;
		if (s->data == data) {
			it = internal::audio_context.playing_samples.erase(it);
			delete s;
		}
		else {
			it++;
		}
	}

	done = false;

	SDL_UnlockMutex(internal::audio_context.mixer_mutex);
}

void Sample::stop()
{
	stop_all();
}

bool Sample::is_playing()
{
	bool playing = false;

	SDL_LockMutex(internal::audio_context.mixer_mutex);

	std::vector<Sample_Instance *>::iterator it;

	for (it = internal::audio_context.playing_samples.begin(); it != internal::audio_context.playing_samples.end(); it++) {
		Sample_Instance *s = *it;
		if (s->data == data) {
			playing = true;
			break;
		}
	}

	SDL_UnlockMutex(internal::audio_context.mixer_mutex);

	return playing;
}

Uint32 Sample::get_length()
{
	return length;
}

int Sample::get_frequency()
{
	return spec->freq;
}

} // End namespace audio

} // End namespace noo
