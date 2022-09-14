#ifdef USE_BASS
#include <bass.h>
#endif

#include "shim4/audio.h"
#include "shim4/error.h"
#include "shim4/mml.h"
#include "shim4/sample.h"
#include "shim4/shim.h"
#include "shim4/util.h"

#include "shim4/internal/audio.h"

//#define DUMP

#ifdef DUMP
SDL_RWops *dumpfile;
#endif

using namespace noo;

static float *music_buf;
static float *sfx_buf;
static SDL_AudioDeviceID audio_device;
static SDL_AudioFormat format;
static int format_bits;
static int format_bytes;
static bool format_is_float;
static bool format_is_signed;
static bool format_should_be_swapped;
static float min_sample;
static float max_sample;
static bool sfx_paused;

static float swap_float(float f)
{
	union {
		uint32_t u;
		float f;
	} u;
	u.f = f;
	u.u = SDL_Swap32(u.u);
	return u.f;
}

static int16_t swap_signed16(int16_t i)
{
	union {
		uint16_t u;
		int16_t i;
	} u;
	u.i = i;
	u.u = SDL_Swap16(u.u);
	return u.i;
}

static int32_t swap_signed32(int32_t i)
{
	union {
		uint32_t u;
		int32_t i;
	} u;
	u.i = i;
	u.u = SDL_Swap32(u.u);
	return u.i;
}

static void write_sample(Uint8 *stream, int i/*sample*/, float v/*min_sample->max_sample*/)
{
	if (format_is_float) {
		if (format_bits == 32) {
			if (format_should_be_swapped) {
				v = swap_float(v);
			}
			*((float *)stream + i) = v;
		}
		else {
			throw util::Error("Only 32 bit floating point samples are supported!");
		}
	}
	else {
		if (format_bits == 8) {
			if (format_is_signed) {
				*((signed char *)stream + i) = v;
			}
			else {
				*((unsigned char *)stream + i) = v;
			}
		}
		else if (format_bits == 16) {
			if (format_is_signed) {
				int16_t ii = v;
				if (format_should_be_swapped) {
					ii = swap_signed16(ii);
				}
				*((int16_t *)stream + i) = ii;
			}
			else {
				uint16_t u = v;
				if (format_should_be_swapped) {
					u = SDL_Swap16(u);
				}
				*((uint16_t *)stream + i) = u;
			}
		}
		else if (format_bits == 32) {
			if (format_is_signed) {
				int32_t ii = v;
				if (format_should_be_swapped) {
					ii = swap_signed32(ii);
				}
				*((int32_t *)stream + i) = ii;
			}
			else {
				throw util::Error("Unsigned 32 bit samples not supported!");
			}
		}
		else {
			throw util::Error("Only 8, 16 and 32 bit samples are supported!");
		}
	}
}

static float read_float_sample(audio::Sample_Instance *s, int sample)
{
	if (s->format_is_float) {
		float v = *((float *)s->data + sample);
		if (s->format_should_be_swapped) {
			v = swap_float(v);
		}
		return v;
	}
	else {
		float v;
		if (s->bits_per_sample == 8) {
			if (s->format_is_signed) {
				v = *((signed char *)s->data + sample);
			}
			else {
				v = *((unsigned char *)s->data + sample);
			}
		}
		else if (s->bits_per_sample == 16) {
			if (s->format_is_signed) {
				v = *((int16_t *)s->data + sample);
			}
			else {
				v = *((uint16_t *)s->data + sample);
			}
		}
		else if (s->bits_per_sample == 32) {
			if (s->format_is_signed) {
				v = *((int32_t *)s->data + sample);
			}
			else {
				throw util::Error("Unsigned 32 bit samples not supported!");
			}
		}
		else {
			throw util::Error(util::string_printf("Sample has %d bits (unsupported!)", s->bits_per_sample));
		}
		if (s->format_is_signed == false) {
			v -= s->max_sample;
		}
		return v / s->max_sample;
	}
}

// Mixes samples and MML into the audio device buffer
static void audio_callback(void *userdata, Uint8 *stream, int stream_length)
{
	SDL_LockMutex(audio::internal::audio_context.mixer_mutex);

	int samples = stream_length / format_bytes / audio::internal::audio_context.device_spec.channels; // 2 channels -- will be repeated to make stereo

	for (int i = 0; i < samples*audio::internal::audio_context.device_spec.channels; i++) {
		music_buf[i] = 0.0f;
		sfx_buf[i] = 0.0f;
	}

	int max = audio::MML::mix(music_buf, samples, sfx_paused);

	std::vector<audio::Sample_Instance *>::iterator it;
	for (it = audio::internal::audio_context.playing_samples.begin(); it != audio::internal::audio_context.playing_samples.end();) {
		audio::Sample_Instance *s = *it;
		if (s->type == audio::SAMPLE_TYPE_SFX && sfx_paused) {
			it++;
			continue;
		}
		int count = s->silence;
		s->silence -= MIN((int)s->silence, samples);
		while (count < samples) {
			int length;
			float p;

			bool interpolate;

			if (s->play_length != s->length) {
				length = s->play_length - s->offset;
				if (length > samples-count) {
					length = samples - count;
				}

				p = (float)s->play_length / s->length;

				interpolate = true;
			}
			else {
				length = s->length - s->offset;
				if (length > samples-count) {
					length = samples - count;
				}

				p = 1.0f;

				interpolate = false;
			}

			max = MAX(max, length);

			for (int i = 0; i < length; i++) {
				int sample_offset = (i + s->offset) * s->channels / p;
				if (sample_offset <= 1 || sample_offset >= (int)s->length) {
					// special case because we can't access the previous sample below (segfault)
					interpolate = false;
				}

				int loops;
				if (s->channels == 2 && audio::internal::audio_context.device_spec.channels == 2) {
					loops = 2;
				}
				else {
					loops = 1;
				}

				for (int k = 0; k < loops; k++) {
					float v;
					if (interpolate) {
						float nsamps = 1.0f / p;
						int g = sample_offset * nsamps;
						if (g < 0) {
							g = 0;
						}
						else if ((Uint32)g >= s->length) {
							g = s->length - 1;
						}
						v = read_float_sample(s, g);
					}
					else {
						v = read_float_sample(s, sample_offset);
					}
					v = v * s->volume * s->master_volume;
					if (s->type != audio::SAMPLE_TYPE_MML) {
						v = v * shim::sfx_volume;
					}

					int dest_offset = (count + i) * audio::internal::audio_context.device_spec.channels + k;

					if (audio::internal::audio_context.device_spec.channels == 2 && s->channels == 1) {
						*((float *)sfx_buf + dest_offset) += v;
						*((float *)sfx_buf + dest_offset+1) += v;
					}
					else {
						*((float *)sfx_buf + dest_offset) += v;
					}

					sample_offset++;
				}
			}

			s->offset += length;

			if (s->loop && s->offset >= s->play_length) {
				s->offset = 0;
			}

			if (s->loop) {
				count += length;
			}
			else {
				break;
			}
		}
		if (s->loop == false && s->offset >= s->play_length) {
			s->sample->set_done(true);
			// erasing causes a memory leak
			it++;// = audio::internal::audio_context.playing_samples.erase(it);
		}
		else {
			it++;
		}
	}

	// Fast paths for common sample formats...
	if (format == AUDIO_F32 && format_should_be_swapped == false) {
		for (int i = 0; i < samples*audio::internal::audio_context.device_spec.channels; i++) {
			float v = (music_buf[i] + sfx_buf[i]);
			if (v < -1.0f) {
				v = -1.0f;
			}
			else if (v > 1.0f) {
				v = 1.0f;
			}
			*((float *)stream + i) = v;
		}
	}
	else if (format == AUDIO_S16 && format_should_be_swapped == false) {
		for (int i = 0; i < samples*audio::internal::audio_context.device_spec.channels; i++) {
			float v = (music_buf[i] + sfx_buf[i]) * max_sample;
			if (v < min_sample) {
				v = min_sample;
			}
			else if (v > max_sample) {
				v = max_sample;
			}
			*((int16_t *)stream + i) = v;
		}
	}
	// generic conversion...
	else {
		for (int i = 0; i < samples*audio::internal::audio_context.device_spec.channels; i++) {
			float v = (music_buf[i] + sfx_buf[i]) * max_sample;
			if (format_is_signed == false) {
				v += -min_sample;
				if (v < 0.0f) {
					v = 0.0f;
				}
				else if (v > max_sample-min_sample) { // subtracting negative = adding positive
					v = max_sample-min_sample;
				}
			}
			else {
				if (v < min_sample) {
					v = min_sample;
				}
				else if (v > max_sample) {
					v = max_sample;
				}
			}
			write_sample(stream, i, v);
		}
	}

#ifdef DUMP
	for (int i = 0; i < max*audio::internal::audio_context.device_spec.channels; i++) {
		float v = (music_buf[i] + sfx_buf[i]);
		if (v < -1.0f) {
			v = -1.0f;
		}
		else if (v > 1.0f) {
			v = 1.0f;
		}
		SDL_WriteLE16(dumpfile, v*32767);
	}
#endif

	SDL_UnlockMutex(audio::internal::audio_context.mixer_mutex);
}

namespace noo {

namespace audio {

bool static_start()
{
	internal::audio_context.playing_samples.clear();
	sfx_paused = false;

	return true;
}

bool start()
{
	shim::music = 0;

	internal::audio_context.mute = util::bool_arg(false, shim::argc, shim::argv, "mute");

	internal::audio_context.mixer_mutex = SDL_CreateMutex();

	if (internal::audio_context.mute == false) {
		int arg;

		SDL_AudioSpec desired;
		SDL_memset(&desired, 0, sizeof(desired));
		if ((arg = util::check_args(shim::argc, shim::argv, "+freq")) > 0) {
			desired.freq = atoi(shim::argv[arg+1]);
		}
		else {
/*#if defined IOS
			desired.freq = 44100;
#else
*/
			desired.freq = 44100;
//#endif
		}
		if ((arg = util::check_args(shim::argc, shim::argv, "+samples")) > 0) {
			desired.samples = atoi(shim::argv[arg+1]);
		}
		else {
			desired.samples = 4096;
		}
		if (util::bool_arg(false, shim::argc, shim::argv, "float-samples")) {
			desired.format = AUDIO_F32;
		}
		else {
			desired.format = AUDIO_S16;
		}
		desired.channels = 2;
		desired.callback = audio_callback;
		desired.userdata = 0;

		// resampling sounds REALLY bad with 48000 Hz isn't used, so don't allow freq changes
		//audio_device = SDL_OpenAudioDevice(0, false, &desired, &internal::audio_context.device_spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_FORMAT_CHANGE);
		audio_device = SDL_OpenAudioDevice(0, false, &desired, &internal::audio_context.device_spec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

		if (audio_device == 0) {
			SDL_DestroyMutex(internal::audio_context.mixer_mutex);
			internal::audio_context.mute = false;
			util::infomsg("audio::start failed: %s\n", SDL_GetError());
			return false;
		}

		shim::samplerate = internal::audio_context.device_spec.freq;

		// this stuff allows converting between formats
		format =  internal::audio_context.device_spec.format;
		format_bits = SDL_AUDIO_BITSIZE(format);
		format_bytes = format_bits / 8;
		format_is_float = SDL_AUDIO_ISFLOAT(format);
		format_is_signed = SDL_AUDIO_ISSIGNED(format);
		format_should_be_swapped = (SDL_AUDIO_ISBIGENDIAN(format) && SDL_BYTEORDER == SDL_LIL_ENDIAN) || (SDL_AUDIO_ISLITTLEENDIAN(format) && SDL_BYTEORDER == SDL_BIG_ENDIAN);
		if (format_is_float) {
			min_sample = -1.0f;
			max_sample = 1.0f;
		}
		else {
			min_sample = -powf(2, format_bits-1);
			max_sample = powf(2, format_bits-1) - 1;
		}

		util::infomsg("Audio format=0x%x, frequency=%d Hz, buffer size=%d samples.\n", format, internal::audio_context.device_spec.freq, internal::audio_context.device_spec.samples);
	}

	music_buf = new float[internal::audio_context.device_spec.samples*internal::audio_context.device_spec.channels];
	sfx_buf = new float[internal::audio_context.device_spec.samples*internal::audio_context.device_spec.channels];
	
	MML::static_start(); // this can't go in audio::static_start because it needs some device info

	if (internal::audio_context.mute == false) {
		SDL_PauseAudioDevice(audio_device, false);
	}

#ifdef USE_BASS
	if (BASS_Init(0, internal::audio_context.device_spec.freq, BASS_DEVICE_MONO, 0, NULL) != TRUE) {
		return false;
	}
#endif

	return true;
}

void end()
{
	std::vector<Sample_Instance *> v = internal::audio_context.playing_samples;
	for (size_t i = 0; i < v.size(); i++) {
		Sample_Instance *s = v[i];
		Sample::stop_instance(s); // this locks mutex
	}

	if (audio_device != 0) {
		SDL_LockMutex(internal::audio_context.mixer_mutex);
		SDL_CloseAudioDevice(audio_device);
		SDL_UnlockMutex(internal::audio_context.mixer_mutex);
	}

	delete[] music_buf;
	delete[] sfx_buf;
	music_buf = 0;
	sfx_buf = 0;

	MML::static_stop();

#ifdef USE_BASS
	BASS_Free();
#endif

	SDL_DestroyMutex(internal::audio_context.mixer_mutex);
}

int millis_to_samples(int millis)
{
	float f = millis / 1000.0f;
	return internal::audio_context.device_spec.freq * f;
}

int samples_to_millis(int samples, int freq)
{
	return samples / (freq == -1 ? (float)internal::audio_context.device_spec.freq : freq) * 1000.0f;
}

void pause_sfx(bool paused)
{
	sfx_paused = paused;
}

namespace internal {

Audio_Context audio_context;

} // End namespace internal

} // End namespace audio

} // End namespace noo
