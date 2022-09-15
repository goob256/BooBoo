#ifndef NOO_MML_H
#define NOO_MML_H

#include "shim4/main.h"
#include "shim4/interp.h"
#include "shim4/sound.h"

namespace noo {

namespace audio {

class Sample;
struct Sample_Instance;

class MML : public Sound {
public:
	enum Wave_Type {
		PULSE = 0,
		NOISE,
		SAWTOOTH,
		SINE,
		TRIANGLE,
		NOISE_ORIG,
	};

	static void static_start();
	static void static_stop();
	static void pause_all();
	static int mix(float *buf, int samples, bool sfx_paused);

	SHIM4_EXPORT MML(SDL_RWops *f, bool load_from_filesystem = false);
	SHIM4_EXPORT MML(std::string filename, bool load_from_filesystem = false);
	SHIM4_EXPORT virtual ~MML();

	SHIM4_EXPORT void play(float volume, bool loop); // Sound interface
	SHIM4_EXPORT void play(bool loop); // plays at SFX volume. This is part of the Sound interface
	SHIM4_EXPORT bool is_done(); // also Sound interface
	SHIM4_EXPORT void stop(); // Sound interface
	SHIM4_EXPORT void pause();
	SHIM4_EXPORT void set_master_volume(float volume);
	SHIM4_EXPORT float get_master_volume();
	SHIM4_EXPORT std::string get_name(); // returns same thing passed to constructor
	SHIM4_EXPORT bool is_playing();

	SHIM4_EXPORT void set_pause_with_sfx(bool pause_with_sfx);
	SHIM4_EXPORT bool pause_with_sfx();

private:
	// Formerly Internal --
	struct Wav_Start {
		int sample;
		Uint32 play_start;
		Uint32 length;
		Sample_Instance *instance;
		float volume;
	};

	struct Reverb_Type {
		int reverberations;
		int falloff_interpolator;
		int falloff_time;
		int start_volume;
		int final_volume;
	};

	class SHIM4_EXPORT Track
	{
	public:
		// pad is # of samples of silence to pad the end with so all tracks are even
		Track(Wave_Type type, std::string text, std::vector< std::pair<int, float> > &volumes, std::vector< std::pair<int, float> > &volume_offsets, std::vector<int> &pitches, std::vector<int> &pitch_offsets, std::vector< std::vector<float> > &pitch_envelopes, std::vector< std::vector<float> > &pitch_offset_envelopes, std::vector< std::pair<int, float> > &dutycycles, int pad, std::vector<Sample *> wav_samples, std::vector<Wav_Start> wav_starts, Uint32 beginning_silence, MML *mml, std::vector<Reverb_Type> reverb_types);
		~Track();

		void play(bool loop);
		void stop();
		void pause();
		int update(float *buf, int length);

		bool is_playing();
		bool is_done();

		void set_master_volume(float master_volume, float master_volume_samples);
		float get_master_volume();
		float get_master_volume_real();

		void set_pause_with_sfx(bool pause_with_sfx);
		bool pause_with_sfx();

	private:
		void reset(Uint32 buffer_fulfilled);

		float vol_from_phase(float p, MML::Wave_Type type, float freq, float dutycycle);
		void generate(float *buf, int samples, int t, const char *tok, int octave);

		void real_get_frequency(int index, std::vector< std::vector<float> > &v, float zero_freq, float default_frequency, float &ret_freq, float &ret_time, float &ret_len, float &last_freq, float &last_start, int &same_sections, bool offset);
		void get_frequency(float start_freq, float &ret_freq, float &ret_time, float &ret_len);
		void get_frequency_offset(float &ret_freq, float &ret_time, float &ret_len);
		float real_get_volume(int &section, std::vector< std::pair<int, float> > &v, bool offset);
		float get_volume();
		float get_dutycycle();
		void start_wavs(Uint32 buffer_offset, Uint32 on_or_after);
		void stop_wavs();
		void set_sample_volumes(float volume);

		std::string next_note(const char *text, int *pos);
		int notelength(const char *tok, const char *text, int *pos);

		Wave_Type type;
		std::string text;
		std::vector< std::pair<int, float> > volumes;
		std::vector< std::pair<int, float> > volume_offsets;
		std::vector<int> pitches;
		std::vector<int> pitch_offsets;
		std::vector< std::vector<float> > pitch_envelopes;
		std::vector< std::vector<float> > pitch_offset_envelopes;
		std::vector< std::pair<int, float> > dutycycles;

		int pad;
		int sample;
		int reset_time;
		int curve_volume;
		int curve_pitch;
		int curve_duty;
		float dutycycle;
		int octave;
		int note_length;
		float volume;
		int tempo;
		int note;
		int volume_section;
		int volume_offset_section;
		int dutycycle_section;
		int pos;
		std::string tok;
		int length_in_samples;
		int note_fulfilled;
		bool done;
		bool padded;
		bool loop;
		bool playing;
		int t;
		float last_freq;
		float last_start;
		int same_sections;
		float last_freq_o;
		float last_start_o;
		int same_sections_o;
		float master_volume;
		float master_volume_samples;
		float mix_volume;
		float last_noise;
		float last_noise2;
		float remain;
		bool fading;
		float prev_time;
		std::vector<Sample *> wav_samples;
		std::vector<Wav_Start> wav_starts; // <sample index, sample to start at>
		int wav_sample;
		bool _pause_with_sfx;

		math::Interpolator *freq_interp;
		math::Interpolator *freq_interp_o;
		int prev_note;
		int prev_note_o;
		int prev_section;
		int prev_section_o;
		math::Interpolator *vol_interp;
		math::Interpolator *vol_interp_o;
		math::Interpolator *duty_interp;

		MML *mml;

		Uint32 beginning_silence;

		float internal_volume;

		std::vector<Reverb_Type> reverb_types;

		int buzz_freq;
		Wave_Type buzz_type;
		float buzz_volume;

		int abs_sample;

		bool no_fade;
	};

	void load(SDL_RWops *f, bool load_from_filesystem);

	std::vector<Track *> tracks;
	std::vector<Track *> reverb_tracks;

	std::vector<Sample *> wav_samples;
	//--

	static std::vector<MML *> loaded_mml;

	std::string name;
	
	bool _pause_with_sfx;
};

void SHIM4_EXPORT play_music(std::string name);
void SHIM4_EXPORT pause_music();
void SHIM4_EXPORT stop_music();

} // End namespace audio

} // End namespace noo

#endif // NOO_MML_H
