#ifndef NOO_SAMPLE_H
#define NOO_SAMPLE_H

#include "shim4/sound.h"

namespace noo {

namespace audio {

const int SAMPLE_TYPE_SFX = 0;
const int SAMPLE_TYPE_MML = 1;
const int SAMPLE_TYPE_USER = 100;

class Sample;

struct Sample_Instance {
	SDL_AudioSpec *spec;
	Uint8 *data;
	Uint32 length;
	Uint32 play_length;
	Uint32 offset;
	Uint32 silence;
	bool loop;
	float volume;
	Sample *sample;
	int bits_per_sample;
	int bytes_per_sample;
	bool format_is_float;
	bool format_is_signed;
	bool format_should_be_swapped;
	float min_sample;
	float max_sample;
	int type; // default 0 (sfx), can set different types
	float master_volume; // different for sfx/MML samples
	int channels;
};

class SHIM4_EXPORT Sample : public Sound {
public:
	static void stop_instance(Sample_Instance *s);

	Sample(std::string filename, bool load_from_filesystem = false);
	virtual ~Sample();

	static void update();

	void play(float volume, bool loop, int type);
	void play(float volume, bool loop); // Sound interface
	void play(bool loop); // Sound interface
	bool is_done(); // Sound interface
	void stop(); // Sound interface, this does a stop_all
	bool is_playing();

	// Play length/silence is in samples based on the device frequency (audio::internal::audio_context.device_spec.freq)
	// If play_length is 0, it plays unstretched
	// silence is samples until it starts
	Sample_Instance *play_stretched(float volume, Uint32 silence, Uint32 play_length, int type = 0);

	void stop_all();

	void set_done(bool done);

	Uint32 get_length();
	int get_frequency();

private:
	void delete_instances();

	SDL_RWops *file;
	SDL_AudioSpec *spec;
	Uint8 *data;
	Uint32 length;
	bool done;
};

} // End namespace audio

} // End namespace noo

#endif // NOO_SAMPLE_H
