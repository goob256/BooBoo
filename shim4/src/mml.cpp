#include "shim4/audio.h"
#include "shim4/error.h"
#include "shim4/mml.h"
#include "shim4/mt.h"
#include "shim4/sample.h"
#include "shim4/shim.h"
#include "shim4/tokenizer.h"
#include "shim4/util.h"

#include "shim4/internal/audio.h"

//#define DUMP

#ifdef DUMP
extern SDL_RWops *dumpfile;
static bool use_mml_loops = true;
static int mml_loops = 15;
#else
static bool use_mml_loops = false;
static int mml_loops = 0;
#endif
static int num_tracks;

using namespace noo;

namespace noo {

namespace audio {

std::vector<MML *> MML::loaded_mml;

const char indexes[7] = { 9, 11, 0, 2, 4, 5, 7 }; // a, b, c, d, e, f, g
static float note_pitches[12][11];

// For the new noise generator
static const float NOISE_CHANGE = 1.0f; // how often to change random value at 44100 Hz note (chosen to sound good)
static float noise_i;
// For the second (original) noise generator
static float noise2_i;
// For all noise
static const int MAX_RAND = 8192; // chosen to sound goodd with original noise generator
static const int HALF_MAX_RAND = MAX_RAND / 2;
static const int NUM_RANDOM_VALUES = 8192;
static int randomness[NUM_RANDOM_VALUES];
static float noise_skip;

static float *tmp;

static void remove_dups(std::vector< std::vector< std::pair<int, float> > > &v)
{
	for (size_t i = 0; i < v.size(); i++) {
		std::vector< std::pair<int, float> >::iterator it;
		for (it = v[i].begin(); it != v[i].end();) {
			std::vector< std::pair<int, float> >::iterator it2 = it;
			it2++;
			if (it2 == v[i].end()) {
				break;
			}
			std::pair<int, float> a = *it;
			std::pair<int, float> b = *it2;
			if (a == b) {
				it = v[i].erase(it);
			}
			else {
				it++;
			}
		}
	}
}

static std::string token(const char *text, int *pos)
{
	char tok[100];
	int i = 0;

	while (*(text+*pos) != 0 && isspace(*(text+*pos))) (*pos)++; // skip whitespace

	// Read the token
	if (*(text+*pos) != 0) {
		tok[i++] = *(text+*pos);
		(*pos)++;
	}

	while (*(text+*pos) != 0 && (isdigit(*(text+*pos)) || *(text+*pos) == '+' || *(text+*pos) == '-' || *(text+*pos) == '.' || (tok[0] == '@' && *(text+*pos) >= 'A' && *(text+*pos) <= 'Z'))) {
		tok[i++] = *(text+*pos);
		(*pos)++;
	}

	while (*(text+*pos) != 0 && isspace(*(text+*pos))) (*pos)++; // skip more whitespace

	tok[i] = 0;
	return tok;
}

// In samples
static int onenotelength(const char *tok, int note_length, int tempo, int octave, int note, char prev_ch)
{
	char ch;
	if (*tok == 'w') {
		ch = prev_ch;
	}
	else {
		ch = *tok;
	}
	int index = -1;
	if (ch >= 'a' && ch <= 'g') {
		index = indexes[ch - 'a'];
		tok++;
		if (*tok == '+') {
			index++;
			tok++;
		}
		else if (*tok == '-') {
			index--;
			tok++;
		}
	}
	else {
		tok++;
	}
	int length = atoi(tok);
	float total = (shim::samplerate / (tempo / 4.0f / 60.0f)) / (length == 0 ? note_length : length);
	float dotlength = total / 2.0f;
	while (*tok != 0) {
		if (*tok == '.') {
			total += dotlength;
			dotlength /= 2.0f;
		}
		tok++;
	}
	return (int)total;
}

void MML::static_start()
{
	for (int note = 0; note < 12; note++) {
		for (int octave = 0; octave < 11; octave++) {
			double freq;
			if (octave == 4 && note == 9) {
				freq = 440.0;
			}
			else {
				int p = (octave - 4) * 12 + note - 9;
				freq = 440.0 * pow(pow(2.0, 1.0 / 12.0), p);
			}
			note_pitches[note][octave] = (float)freq;
		}
	}

	util::srand(0);

	for (int i = 0; i < NUM_RANDOM_VALUES; i++) {
		randomness[i] = util::rand(0, MAX_RAND-1);
	}

	util::srand((uint32_t)time(nullptr));

	noise_skip = 44100.0f / internal::audio_context.device_spec.freq;
	noise_i = 0.0f;
	noise2_i = 1.0f;

	tmp = new float[internal::audio_context.device_spec.samples*internal::audio_context.device_spec.channels];

	loaded_mml.clear();
}

void MML::static_stop()
{
	delete[] tmp;
}

void MML::pause_all()
{
	SDL_LockMutex(internal::audio_context.mixer_mutex);

	for (size_t i = 0; i < loaded_mml.size(); i++) {
		for (size_t j = 0; j < loaded_mml[i]->tracks.size(); j++) {
			loaded_mml[i]->tracks[j]->pause();
		}
		for (size_t j = 0; j < loaded_mml[i]->reverb_tracks.size(); j++) {
			loaded_mml[i]->reverb_tracks[j]->pause();
		}
	}

	SDL_UnlockMutex(internal::audio_context.mixer_mutex);
}

MML::Track::Track(Wave_Type type, std::string text, std::vector< std::pair<int, float> > &volumes, std::vector< std::pair<int, float> > &volume_offsets, std::vector<int> &pitches, std::vector<int> &pitch_offsets, std::vector< std::vector<float> > &pitch_envelopes, std::vector< std::vector<float> > &pitch_offset_envelopes, std::vector< std::pair<int, float> > &dutycycles, int pad, std::vector<Sample *> wav_samples, std::vector<Wav_Start> wav_starts, Uint32 beginning_silence, MML *mml, std::vector<Reverb_Type> reverb_types) :
	type(type),
	text(text),
	volumes(volumes),
	volume_offsets(volume_offsets),
	pitches(pitches),
	pitch_offsets(pitch_offsets),
	pitch_envelopes(pitch_envelopes),
	pitch_offset_envelopes(pitch_offset_envelopes),
	dutycycles(dutycycles),
	pad(pad),
	playing(false),
	master_volume(1.0f),
	master_volume_samples(1.0f),
	wav_samples(wav_samples),
	wav_starts(wav_starts),
	_pause_with_sfx(true),
	mml(mml),
	beginning_silence(beginning_silence),
	internal_volume(1.0f),
	reverb_types(reverb_types)
{
	freq_interp = nullptr;
	freq_interp_o = nullptr;
	vol_interp = nullptr;
	vol_interp_o = nullptr;
	duty_interp = nullptr;

	abs_sample = 0;

	reset(0);
}

MML::Track::~Track()
{
	stop_wavs();
	delete freq_interp;
	delete freq_interp_o;
	delete vol_interp;
	delete vol_interp_o;
	delete duty_interp;
}

void MML::Track::play(bool loop)
{
	if (playing) {
		stop();
	}

	done = false;

	playing = true;

	this->loop = loop;

	stop_wavs();
	start_wavs(0, sample);
}

void MML::Track::stop()
{
	playing = false;
	reset(0);
}

void MML::Track::pause()
{
	playing = false;
	stop_wavs();
}

int MML::Track::update(float *buf, int length)
{
	if (done) {
		return 0;
	}

	set_sample_volumes(master_volume_samples);

	int buffer_fulfilled = 0;

	while (buffer_fulfilled < length) {
		if (sample < 0) {
			int samples = MIN(length, -sample);
			samples = MIN(samples, length - buffer_fulfilled);
			buffer_fulfilled += samples;
			sample += samples;
		}
		int to_generate = length_in_samples - note_fulfilled;
		int left_in_buffer = length - buffer_fulfilled;
		if (left_in_buffer < to_generate) {
			to_generate = left_in_buffer;
		}
		generate(buf + buffer_fulfilled * internal::audio_context.device_spec.channels, to_generate, t, tok.c_str(), octave);
		t += to_generate;
		buffer_fulfilled += to_generate;
		bool get_next_note = false;
		if (note_fulfilled >= length_in_samples) {
			get_next_note = true;
		}
		if (get_next_note) {
			const char *text_cstr = text.c_str();

			std::string last_tok = tok;

			tok = next_note(text_cstr, &pos);

			if (tok[0] != 0) {
				t = 0;
			}

			note++;

			if (tok[0] == 0) {
				note--;
				if (padded) {
					if (loop) {
						int save = buffer_fulfilled;
						reset(buffer_fulfilled);
						buffer_fulfilled = save;
					}
					else {
						// Silence at the end, don't need to do anything
						if (loop == false) {
							// don't 'stop' reverb tracks which would reset them, just set them done for mix to clean up
							std::string tag = "@X";
							if (text.find(tag) == std::string::npos) {
								stop();
							}
						}
						// reset (stop?) above sets done to false
						done = true;

						return buffer_fulfilled;
					}
				}
				else {
					if (use_mml_loops) {
						std::string tag = "@X";
						if (text.find(tag) == std::string::npos) {
							mml_loops--;
						}
						// FIXME:
						//if (mml_loops < num_tracks) loop = false; else loop = true;
						if (mml_loops < num_tracks) exit(0); else loop = true;
					}
					length_in_samples = pad;
					tok = "r";
					padded = true;
				}
			}
			else {
				length_in_samples = notelength(tok.c_str(), text_cstr, &pos);
			}
			note_fulfilled = 0;
		}
	}

	return buffer_fulfilled;
}

bool MML::Track::is_playing()
{
	return playing;
}

bool MML::Track::is_done()
{
	return done;
}

void MML::Track::reset(Uint32 buffer_fulfilled)
{
	stop_wavs();
	if (playing) {
		start_wavs(buffer_fulfilled, 0);
	}

	wav_sample = -1;
	// some of this stuff must be before the 'next_note' call below
	sample = beginning_silence > 0 ? -int(beginning_silence) : 0;
	reset_time = 0;
	curve_volume = 0;
	curve_pitch = 0;
	curve_duty = 0;
	dutycycle = 0.5f;
	octave = 4;
	note_length = 4;
	volume = 1.0f;
	tempo = 120;
	note = 0;
	volume_section = 0;
	volume_offset_section = 0;
	dutycycle_section = 0;
	t = 0;
	pos = 0;
	mix_volume = 1.0f;
	no_fade = false;

	buzz_freq = 0;
	buzz_type = PULSE;
	buzz_volume = 1.0f/255.0f;
	
	tok = next_note(text.c_str(), &pos);
	length_in_samples = notelength(tok.c_str(), text.c_str(), &pos);
	note_fulfilled = 0;
	padded = false;
	done = false;
	last_freq = -1;
	last_start = 0;
	same_sections = 0;
	last_freq_o = -1;
	last_start_o = 0;
	same_sections_o = 0;
	last_noise = 0.0f;
	last_noise2 = 0.0f;

	// stuff for fading first two/last 2.x phases
	//--
	remain = 0.0f;
	fading = false;
	prev_time = 0.0f;
	//--

	prev_note = 0;
	prev_note_o = 0;
	prev_section = 0;
	prev_section_o = 0;

	std::vector<int> used_reverbs;
	for (size_t i = 0; i < reverb_types.size(); i++) {
		std::string tag = "@R" + util::itos((int)i);
		if (text.find(tag) != std::string::npos) {
			if (std::find(used_reverbs.begin(), used_reverbs.end(), (int)i) == std::end(used_reverbs)) {
				used_reverbs.push_back((int)i);
			}
		}
	}

	for (size_t i = 0; i < used_reverbs.size(); i++) {
		int rtype = used_reverbs[i];
		bool active = false;
		std::string new_text;
		for (size_t j = 0; j < text.length(); j++) {
			if (text[j] == '@' && j < text.length()-2 && text[j+1] == 'R' && isdigit(text[j+2])) {
				int rnum = atoi(text.c_str()+j+2);
				if (rnum == rtype) {
					active = !active;
				}
			}
			char s[2];
			s[1] = 0;
			if (active == false && text[j] >= 'a' && text[j] <= 'g') {
				s[0] = 'r';
			}
			else if (j > 0 && text[j-1] == '@' && text[j] == 'R' && j < text.length()-1 && isdigit(text[j+1])) {
				s[0] = 'X'; // killed '@R' so it doesn't propagate and repeat
			}
			else {
				s[0] = text[j];
			}
			new_text += s;
		}
		Reverb_Type rt = reverb_types[rtype];
		math::Interpolator *tinterp;
		math::Interpolator *vinterp;
		switch (rt.falloff_interpolator) {
			case 1:
				// min. 4 points for Hermite
				tinterp = new math::I_Linear;
				break;
			case 2:
				tinterp = new math::I_Slow();
				break;
			case 3:
				tinterp = new math::I_Sin();
				break;
			case 4:
				tinterp = new math::I_Pulse();
				break;
			default:
				tinterp = new math::I_Linear();
				break;
		}
		switch (rt.falloff_interpolator) {
			case 1:
				// min. 4 points for Hermite
				vinterp = new math::I_Linear;
				break;
			case 2:
				vinterp = new math::I_Slow();
				break;
			case 3:
				vinterp = new math::I_Sin();
				break;
			case 4:
				vinterp = new math::I_Pulse();
				break;
			default:
				vinterp = new math::I_Linear();
				break;
		}
		tinterp->start(0, 0, rt.falloff_time, rt.falloff_time, rt.reverberations+1);
		tinterp->interpolate(1); // there is a +1 above to skip the 0 spot (start value above)
		vinterp->start(rt.start_volume, rt.start_volume, rt.final_volume, rt.final_volume, rt.reverberations);
		for (int i = 0; i < rt.reverberations; i++) {
			tinterp->interpolate(1);
			int delay = tinterp->get_value();
			// convert from MS to samples
			delay *= shim::samplerate / 1000.0f;
			delay += buffer_fulfilled;
			vinterp->interpolate(1);
			float v = vinterp->get_value()/255.0f;
			std::vector<Wav_Start> w; // dummy
			Track *t = new Track(type, new_text, volumes, volume_offsets, pitches, pitch_offsets, pitch_envelopes, pitch_offset_envelopes, dutycycles, 0, wav_samples, w, delay, mml, reverb_types);
			t->internal_volume = v;
			mml->reverb_tracks.push_back(t);
			t->set_pause_with_sfx(mml->_pause_with_sfx);
			for (size_t i = 0; i < mml->reverb_tracks.size(); i++) {
				mml->reverb_tracks[i]->set_master_volume(master_volume_samples/mml->reverb_tracks.size(), master_volume_samples);
			}
			if (playing) {
				t->play(false);
			}
		}
		delete tinterp;
		delete vinterp;
	}
}

float MML::Track::vol_from_phase(float p, MML::Wave_Type type, float freq, float dutycycle)
{
	float v;

	switch (type) {
		case MML::PULSE: {
			if (p < dutycycle) {
				v = 1.0f;
			}
			else {
				v = -1.0f;
			}
			break;
		}
		case MML::NOISE: {
			float noise_change = NOISE_CHANGE * (44100.0f / freq);
			if (note_fulfilled == 0) {
				noise_i = noise_change;
			}
			if (noise_i >= noise_change) {
				int o = note_fulfilled % NUM_RANDOM_VALUES;
				int r = randomness[o];
				last_noise = (float)r / HALF_MAX_RAND - 1;
				noise_i -= noise_change;
			}
			else {
				noise_i += noise_skip;
			}
			v = last_noise;
			break;
		}
		case MML::NOISE_ORIG: {
			if (noise2_i >= 1.0f) {
				int o = note_fulfilled % NUM_RANDOM_VALUES;
				int r1 = randomness[NUM_RANDOM_VALUES-o-1];
				if (r1 < freq) {
					int r2 = randomness[o];
					last_noise2 = (float)r2 / HALF_MAX_RAND - 1;
				}
				noise2_i -= 1.0f;
			}
			else {
				noise2_i += noise_skip;
			}
			v = last_noise2;
			break;
		}
		case MML::SAWTOOTH: {
			v = p * 2 - 1;
			break;
		}
		case MML::SINE: {
			v = sin(p*M_PI*2);
			break;
		}
		case MML::TRIANGLE: {
			v = (1.0f - fabsf(p - 0.5f)) * 4 - 3;
			break;
		}
		default:
			v = 0;
			break;
	};

	return v;
}

void MML::Track::generate(float *buf, int samples, int t, const char *tok, int octave)
{
	char c = tok[0];
	bool mute;

	if (c == 'r') {
		mute = true;
	}
	else {
		mute = false;
	}

	int index = 0;

	if (mute == false) {
		index = indexes[c-'a'];
		if (tok[1] == '+') {
			index++;
		}
		else if (tok[1] == '-') {
			index--;
		}
	}

	float frequency = note_pitches[index][octave];

	for (int i = 0; i < samples; i++) {
		if (wav_sample >= 0) {
			note_fulfilled++;
			sample++;
			abs_sample++;
			continue;
		}

		float freq1, freq2;
		float time1, time2;
		float len1, len2;
		get_frequency(frequency, freq1, time1, len1);
		get_frequency_offset(freq2, time2, len2);

		float freq = freq1 + freq2;
		float time = (time1 == note_fulfilled) ? time2 : time1;
		float len = (len1 == length_in_samples) ? len2 : len1;

		if (freq < 0) {
			throw util::Error("Frequency < 0 in MML! (probably not desired)");
		}

		float add;

		switch (type) {
			case TRIANGLE:
			case SAWTOOTH: {
				add = shim::samplerate/4.0f;
				break;
			}
			default: {
				add = 0;
				break;
			}
		}

		float p;

		switch (type) {
			case NOISE: 
			case NOISE_ORIG: 
			{
				p = 0.0f;
				break;
			}
			default: {
				p = fmodf((time * freq + add) / shim::samplerate, 1.0f);
				break;
			}
		}

		float duty = get_dutycycle();

		float v = vol_from_phase(p, type, freq, duty);

		const float max_fade = 512.0f; // max samples to fade in/out
		float diff = len - time;
		if (fading == false) {
			// this gets number of trailing samples (2 phases plus whatever is dangling)
			remain = time * freq / shim::samplerate; // number of phases in time
			remain = fmodf(remain, 1.0f) + 2.0f; // 2 phases plus dangling
			remain = remain * shim::samplerate / freq; // dangling samples (2.x phases)

			remain = MIN(max_fade, remain);

			if (no_fade == false && diff < remain) {
				fading = true;
			}
		}
		else {
			if (time < prev_time) {
				fading = false;
			}
		}
		
		prev_time = time;

		float fade_v;
		if (fading) {
			fade_v = diff/remain;
		}
		else {
			fade_v = 1.0f;
		}
		float volume = get_volume() * fade_v * master_volume;

		float v_final = MAX(-1.0f, MIN(1.0f, v * volume)) * mix_volume * internal_volume;

		if (buzz_freq > 0 && fabsf(v_final) >= 0.05f) {
			bool sub;
			if (type == PULSE) {
				if (v == 1.0f) {
					sub = true;
				}
				else {
					sub = false;
				}
			}
			else {
				if (p < 0.5f) {
					sub = true;
				}
				else {
					sub = false;
				}
			}
			int bf = buzz_freq;
			if (bf % 2 == 1) {
				bf++;
			}
			bf--;
			float f = 1.0f / bf;
			float p2 = fmod(p, f);
			float p3 = p2 / f;
			float v = vol_from_phase(p3, buzz_type, freq, duty);
			v *= buzz_volume * master_volume * mix_volume * internal_volume;
			if (sub) {
				v_final -= buzz_volume/2.0f*internal_volume;
				v_final += v;
			}
			else {
				v_final += buzz_volume/2.0f*internal_volume;
				v_final += v;
			}
		}

		if (mute) {
			v_final = 0.0f;
		}

		if (internal::audio_context.device_spec.channels == 2) {
			buf[i*2] += v_final;
			buf[i*2+1] += v_final;
		}
		else {
			buf[i] += v_final;
		}

		note_fulfilled++;
		sample++;
		abs_sample++;
	}
}

void MML::Track::real_get_frequency(int index, std::vector< std::vector<float> > &v, float zero_freq, float default_freq, float &ret_freq, float &ret_time, float &ret_len, float &last_freq, float &last_start, int &same_sections, bool offset)
{
	if (index == -1) {
		ret_freq = default_freq;
		ret_time = note_fulfilled;
		ret_len = length_in_samples;
		return;
	}
	int sz = (int)v[index].size();
	int i = (sz-1.0f) * note_fulfilled / length_in_samples;

	int *pn;
	int *ps;
	math::Interpolator **interp;
	if (offset) {
		interp = &freq_interp_o;
		pn = &prev_note_o;
		ps = &prev_section_o;
	}
	else {
		interp = &freq_interp;
		pn = &prev_note;
		ps = &prev_section;
	}

	if (note != *pn) {
		*pn = note;
		*ps = 0;
		delete *interp;
		*interp = nullptr;
	}
	else if (*ps != i) {
		*ps = i;
		delete *interp;
		*interp = nullptr;
	}

	float start_freq;
	float prev_freq, next_freq;
	if (i == 0) {
		start_freq = zero_freq;
		prev_freq = zero_freq;
	}
	else {
		start_freq = v[index][i];
		prev_freq = v[index][i-1];
	}
	float end_freq = v[index][i+1];
	if ((int)v[index].size() > i+2) {
		next_freq = v[index][i+2];
	}
	else {
		next_freq = end_freq;
	}
	float stride = length_in_samples / (sz-1.0f);
	float start = i * stride;
	float diff = note_fulfilled - start;
	bool delete_it = false;
	switch (curve_pitch) {
		case 1:
			if (dynamic_cast<math::I_Hermite *>(*interp) == nullptr) {
				delete_it = true;
			}
			break;
		case 2:
			if (dynamic_cast<math::I_Slow *>(*interp) == nullptr) {
				delete_it = true;
			}
			break;
		case 3:
			if (dynamic_cast<math::I_Sin *>(*interp) == nullptr) {
				delete_it = true;
			}
			break;
		case 4:
			if (dynamic_cast<math::I_Pulse *>(*interp) == nullptr) {
				delete_it = true;
			}
			break;
		default:
			if (dynamic_cast<math::I_Linear *>(*interp) == nullptr) {
				delete_it = true;
			}
			break;
	}
	if (delete_it ) {
		delete *interp;
		*interp = nullptr;
	}
	if (note_fulfilled == 0 || diff == 0 || *interp == nullptr) {
		delete *interp;
		// special case
		/*
		int cp = curve_pitch;
		if (start_freq == end_freq) {
			cp = 0;
		}
		switch (cp) {
		*/
		switch (curve_pitch) {
			case 1:
				*interp = new math::I_Hermite();
				break;
			case 2:
				*interp = new math::I_Slow();
				break;
			case 3:
				*interp = new math::I_Sin();
				break;
			case 4:
				*interp = new math::I_Pulse();
				break;
			default:
				*interp = new math::I_Linear();
				break;
		}
		(*interp)->start(prev_freq, start_freq, end_freq, next_freq, stride);
	}
	(*interp)->interpolate(1);
	ret_freq = (*interp)->get_value();
	if (reset_time != 0) {
		int period = stride * reset_time;
		ret_time = note_fulfilled % period;
		ret_len = period;
	}
	/*
	if (reset_time == 2) {
		ret_time = abs_sample;
		ret_len = FLT_MAX;
	}
	*/
	//else if (reset_time == 1) {
	else {
		if (last_freq == start_freq) {
			ret_time = note_fulfilled - last_start;
			ret_len = stride * same_sections;
		}
		else {
			last_freq = start_freq;
			last_start = start;
			same_sections = 1;
			for (int j = i+1; j < sz; j++) {
				if (start_freq != v[index][j]) {
					break;
				}
				same_sections++;
			}
			ret_time = diff;
			if (same_sections > 1) {
				ret_len = same_sections * stride;
			}
			else {
				ret_len = stride;
			}
		}
	}
	/*
	else {
		ret_time = note_fulfilled;
		ret_len = length_in_samples;
	}
	*/
}


void MML::Track::get_frequency(float start_freq, float &ret_freq, float &ret_time, float &ret_len)
{
	//int index = pitches[note];
	//real_get_frequency(pitches[note], pitch_envelopes, index < 0 ? start_freq : pitch_envelopes[index][0], start_freq, ret_freq, ret_time, ret_len, last_freq, last_start, same_sections, false);
	real_get_frequency(pitches[note], pitch_envelopes, start_freq, start_freq, ret_freq, ret_time, ret_len, last_freq, last_start, same_sections, false);
}

void MML::Track::get_frequency_offset(float &ret_freq, float &ret_time, float &ret_len)
{
	int index = pitch_offsets[note];
	real_get_frequency(index, pitch_offset_envelopes, index < 0 ? 0.0f : pitch_offset_envelopes[index][0], 0.0f, ret_freq, ret_time, ret_len, last_freq_o, last_start_o, same_sections_o, true);
	//real_get_frequency(pitch_offsets[note], pitch_offset_envelopes, 0.0f, 0.0f, ret_freq, ret_time, ret_len, last_freq_o, last_start_o, same_sections_o, true);
}

float MML::Track::real_get_volume(int &section, std::vector< std::pair<int, float> > &v, bool offset)
{
	math::Interpolator **interp;
	if (offset) {
		interp = &vol_interp_o;
	}
	else {
		interp = &vol_interp;
	}
	int sz = (int)v.size();
	while (section < sz-1 && sample >= v[section].first) {
		section++;
		delete *interp;
		*interp = nullptr;
	}
	float stride = v[section].first - v[section-1].first;
	float diff = sample - v[section-1].first;
	float prev_v;
	if (section-2 >= 0) {
		prev_v = v[section-2].second;
	}
	else {
		prev_v = v[section-1].second;
	}
	float start_v = v[section-1].second;
	float end_v = v[section].second;
	float next_v;
	if ((int)v.size() > section+1) {
		next_v = v[section+1].second;
	}
	else {
		next_v = end_v;
	}
	bool delete_it = false;
	switch (curve_volume) {
		case 1:
			if (dynamic_cast<math::I_Hermite *>(*interp) == nullptr) {
				delete_it = true;
			}
			break;
		case 2:
			if (dynamic_cast<math::I_Slow *>(*interp) == nullptr) {
				delete_it = true;
			}
			break;
		case 3:
			if (dynamic_cast<math::I_Sin *>(*interp) == nullptr) {
				delete_it = true;
			}
			break;
		case 4:
			if (dynamic_cast<math::I_Pulse *>(*interp) == nullptr) {
				delete_it = true;
			}
			break;
		default:
			if (dynamic_cast<math::I_Linear *>(*interp) == nullptr) {
				delete_it = true;
			}
			break;
	}
	if (delete_it ) {
		delete *interp;
		*interp = nullptr;
	}
	if (diff == 0 || *interp == nullptr) {
		delete *interp;
		// special case
		int cv = curve_volume;
		if (start_v == end_v) {
			cv = 0;
		}
		switch (cv) {
			case 1:
				*interp = new math::I_Hermite();
				break;
			case 2:
				*interp = new math::I_Slow();
				break;
			case 3:
				*interp = new math::I_Sin();
				break;
			case 4:
				*interp = new math::I_Pulse();
				break;
			default:
				*interp = new math::I_Linear();
				break;
		}
		(*interp)->start(prev_v, start_v, end_v, next_v, stride);
	}
	(*interp)->interpolate(1);
	return (*interp)->get_value();
}

float MML::Track::get_volume()
{
	return real_get_volume(volume_section, volumes, false) + real_get_volume(volume_offset_section, volume_offsets, true);
}

float MML::Track::get_dutycycle()
{
	while (dutycycle_section < int(dutycycles.size())-1 && sample >= dutycycles[dutycycle_section].first) {
		dutycycle_section++;
		delete duty_interp;
		duty_interp = nullptr;
	}
	float stride = dutycycles[dutycycle_section].first - dutycycles[dutycycle_section-1].first;
	float diff = sample - dutycycles[dutycycle_section-1].first;
	float prev_duty;
	if (dutycycle_section-2 >= 0) {
		prev_duty = dutycycles[dutycycle_section-2].second;
	}
	else {
		prev_duty = dutycycles[dutycycle_section-1].second;
	}
	float start_duty = dutycycles[dutycycle_section-1].second;
	float end_duty = dutycycles[dutycycle_section].second;
	float next_duty;
	if ((int)dutycycles.size() > dutycycle_section+1) {
		next_duty = dutycycles[dutycycle_section+1].second;
	}
	else {
		next_duty = end_duty;
	}
	bool delete_it = false;
	switch (curve_duty) {
		case 1:
			if (dynamic_cast<math::I_Hermite *>(duty_interp) == nullptr) {
				delete_it = true;
			}
			break;
		case 2:
			if (dynamic_cast<math::I_Slow *>(duty_interp) == nullptr) {
				delete_it = true;
			}
			break;
		case 3:
			if (dynamic_cast<math::I_Sin *>(duty_interp) == nullptr) {
				delete_it = true;
			}
			break;
		case 4:
			if (dynamic_cast<math::I_Pulse *>(duty_interp) == nullptr) {
				delete_it = true;
			}
			break;
		default:
			if (dynamic_cast<math::I_Linear *>(duty_interp) == nullptr) {
				delete_it = true;
			}
			break;
	}
	if (delete_it ) {
		delete duty_interp;
		duty_interp = nullptr;
	}
	if (diff == 0 || duty_interp == nullptr) {
		delete duty_interp;
		// special case
		int cd = curve_duty;
		if (start_duty == end_duty) {
			cd = 0;
		}
		switch (cd) {
			case 1:
				duty_interp = new math::I_Hermite();
				break;
			case 2:
				duty_interp = new math::I_Slow();
				break;
			case 3:
				duty_interp = new math::I_Sin();
				break;
			case 4:
				duty_interp = new math::I_Pulse();
				break;
			default:
				duty_interp = new math::I_Linear();
				break;
		}
		duty_interp->start(prev_duty, start_duty, end_duty, next_duty, stride);
	}
	duty_interp->interpolate(1);
	return duty_interp->get_value();
}

std::string MML::Track::next_note(const char *text, int *pos)
{
	std::string result;
	bool done = false;
	do {
		result = token(text, pos);
		switch (result[0]) {
			case '<':
			case '>':
				if (result[0] == '>') {
					octave--;
				}
				else {
					octave++;
				}
				if (octave < 0) {
					octave = 0;
				}
				else if (octave > 11) {
					octave = 11;
				}
				break;
			case 'o':
				octave = atoi(result.c_str()+1);
				break;
			case 'w':
				break;
			case 'l':
				note_length = atoi(result.c_str()+1);
				break;
			case 't':
				tempo = atoi(result.c_str() + 1);
				break;
			case '@':
				if (!strncmp(result.c_str()+1, "TYPE", 4)) {
					type = (Wave_Type)atoi(result.c_str() + 5);
				}
				else if (!strncmp(result.c_str()+1, "CV", 2)) {
					curve_volume = atoi(result.c_str() + 3);
				}
				else if (!strncmp(result.c_str()+1, "CP", 2)) {
					curve_pitch = atoi(result.c_str() + 3);
				}
				else if (!strncmp(result.c_str()+1, "CD", 2)) {
					curve_duty = atoi(result.c_str() + 3);
				}
				else if (!strncmp(result.c_str()+1, "RT", 2)) {
					reset_time = atoi(result.c_str() + 3);
				}
				else if (!strncmp(result.c_str()+1, "S", 1)) {
					int num = atoi(result.c_str() + 2);
					if (num == wav_sample) {
						wav_sample = -1;
					}
					else {
						wav_sample = num;
					}
				}
				else if (!strncmp(result.c_str()+1, "BF", 2)) {
					buzz_freq = atoi(result.c_str()+3);
				}
				else if (!strncmp(result.c_str()+1, "BTYPE", 5)) {
					buzz_type = (Wave_Type)atoi(result.c_str() + 6);
				}
				else if (!strncmp(result.c_str()+1, "BV", 2)) {
					buzz_volume = atoi(result.c_str()+3) / 255.0f;
				}
				else if (!strncmp(result.c_str()+1, "F", 1)) {
					no_fade = !((bool)atoi(result.c_str() + 2));
				}
				break;
			case 'v':
				// doesn't need to do this -- volumes set with get_volume
				break;
			case 'm': // mix volume
				mix_volume = atoi(result.c_str() + 1) / 255.0f;
				break;
			case 'y':
				// doesn't need to do this -- dutycycles set with get_dutycycle
				break;
			default:
				if (result[0] == 0 || result[0] == 'r' || (result[0] >= 'a' && result[0] <= 'g')) {
					done = true;
				}
				break;
		}
	} while (!done);
	return result;
}

// adds waits to length
int MML::Track::notelength(const char *tok, const char *text, int *pos)
{
	char ch = *tok;
	int total = onenotelength(tok, note_length, tempo, octave, note, 'z'); // z == nothing never used
	int pos2 = *pos;
	std::string tok2;
	do {
		tok2 = token(text, &pos2);
		if (tok2[0] == 'w') {
			total += onenotelength(tok2.c_str(), note_length, tempo, octave, note, ch);
		}
	} while (tok2[0] == 'w');
	return total;
}

void MML::Track::set_master_volume(float master_volume, float master_volume_samples)
{
	this->master_volume = master_volume;

	int nr = int(mml->reverb_tracks.size());
	bool is_reverb = false;
	for (int i = 0; i < nr; i++) {
		if (this == mml->reverb_tracks[i]) {
			is_reverb = true;
			break;
		}
	}
	if (is_reverb == false) {
		for (int i = 0; i < nr; i++) {
			mml->reverb_tracks[i]->set_master_volume(master_volume_samples/nr, master_volume_samples);
		}
	}

	this->master_volume_samples = master_volume_samples;
	set_sample_volumes(master_volume_samples);
}

float MML::Track::get_master_volume()
{
	return master_volume_samples;
}

float MML::Track::get_master_volume_real()
{
	return master_volume;
}

void MML::Track::set_pause_with_sfx(bool pause_with_sfx)
{
	_pause_with_sfx = pause_with_sfx;
}

bool MML::Track::pause_with_sfx()
{
	return _pause_with_sfx;
}

void MML::Track::start_wavs(Uint32 buffer_offset, Uint32 on_or_after)
{
	for (size_t i = 0; i < wav_starts.size(); i++) {
		Wav_Start &w = wav_starts[i];
		if (w.play_start >= on_or_after) {
			w.instance = wav_samples[w.sample]->play_stretched(w.volume*internal_volume, w.play_start+buffer_offset-on_or_after, w.length, (_pause_with_sfx == false) ? SAMPLE_TYPE_MML : SAMPLE_TYPE_SFX);
			w.instance->master_volume = master_volume_samples;
		}
	}
}

void MML::Track::stop_wavs()
{
	for (size_t i = 0; i < wav_starts.size(); i++) {
		Sample_Instance *instance = wav_starts[i].instance;
		if (instance != 0) {
			Sample::stop_instance(instance);
			wav_starts[i].instance = 0;
		}
	}
}

void MML::Track::set_sample_volumes(float volume)
{
	// Update sample volumes with shim::music volume as they're part of the music (these have type SAMPLE_TYPE_MML so don't get sfx_volume)
	for (size_t i = 0; i < wav_starts.size(); i++) {
		Wav_Start &w = wav_starts[i];
		if (w.instance != 0) {
			w.instance->master_volume = volume;
		}
	}
}

//--

int MML::mix(float *buf, int samples, bool sfx_paused)
{
	size_t i;

	int max = 0;

	try {

	for (i = 0; i < loaded_mml.size(); i++) {
		for (int j = 0; j < samples*internal::audio_context.device_spec.channels; j++) {
			tmp[j] = 0.0f;
		}

		std::vector<Track *> &tracks = loaded_mml[i]->tracks;

		int num_playing_tracks = 0;

		for (size_t track = 0; track < tracks.size(); track++) {
			if (tracks[track]->is_playing()) {
				if (tracks[track]->pause_with_sfx() && sfx_paused) {
					continue;
				}
				int fulfilled = tracks[track]->update(tmp, samples);
				if (fulfilled > 0) {
					num_playing_tracks++;
					max = MAX(max, fulfilled);
				}
			}
		}
		
		std::vector<Track *> &reverb_tracks = loaded_mml[i]->reverb_tracks;

		for (std::vector<Track *>::iterator it = reverb_tracks.begin(); it != reverb_tracks.end();) {
			Track *t = *it;
			if (t->is_playing()) {
				if (t->pause_with_sfx() && sfx_paused) {
					it++;
					continue;
				}
				int fulfilled = t->update(tmp, samples);
				if (fulfilled > 0) {
					num_playing_tracks++;
					max = MAX(max, fulfilled);
				}
				if (t->is_done()) {
					it = reverb_tracks.erase(it);
					delete t;
					for (size_t i = 0; i < reverb_tracks.size(); i++) {
						reverb_tracks[i]->set_master_volume(tracks[0]->get_master_volume()/reverb_tracks.size(), tracks[0]->get_master_volume());
					}
				}
				else {
					it++;
				}
			}
			else {
				it++;
			}
		}

		if (num_playing_tracks > 0) {
			for (int j = 0; j < samples*internal::audio_context.device_spec.channels; j++) {
				buf[j] += tmp[j];
			}
		}
	}

	}
	catch (util::Error &e) {
		util::errormsg("MML failure (%s, %s).\n", e.error_message.c_str(), loaded_mml[i]->get_name().c_str());
		throw;
	}

	return max;
}

MML::MML(std::string filename, bool load_from_filesystem) :
	name(filename)
{
	if (load_from_filesystem == false) {
		filename = "audio/mml/" + filename;
	}

	SDL_RWops *f;

	int sz = 0;
	if (load_from_filesystem) {
		f = SDL_RWFromFile(filename.c_str(), "r");
		if (f) {
			sz = (int)SDL_RWsize(f);
		}
	}
	else {
		f = util::open_file(filename, &sz);
	}

	if (f == 0) {
		throw util::LoadError("Error loading MML");
	}

	load(f, load_from_filesystem);
}

MML::MML(SDL_RWops *f, bool load_from_filesystem) :
	_pause_with_sfx(true)
{
	load(f, load_from_filesystem);
}

void MML::load(SDL_RWops *f, bool load_from_filesystem)
{
	std::vector<std::string> tracks_s;
	std::vector< std::vector< std::pair<int, float> > > volumes;
	std::vector< std::vector< std::pair<int, float> > > volume_offsets;
	std::vector< std::vector< std::pair<int, float> > > dutycycles;
	std::vector< std::vector<int> > pitches;
	std::vector< std::vector<int> > pitch_offsets;
	std::vector<int> note_lengths;
	std::vector<int> tempos;
	std::vector<int> octaves;
	std::vector<int> sample; // samples read
	std::vector<int> vol_start; // volume envelope start sample
	std::vector<int> vol_o_start; // volume offset envelope start sample
	std::vector<int> curr_vol; // current volume envelope
	std::vector<int> curr_vol_o; // current volume offset envelope
	std::vector<int> duty_start; // dutycycle envelope start sample
	std::vector<int> curr_duty; // current dutycycle envelope
	std::vector<int> note;
	std::vector<int> current_pitch;
	std::vector<int> current_pitch_o;
	std::vector<char> prev_ch;
	std::vector< std::vector<float> > volume_envelopes;
	std::vector< std::vector<float> > volume_offset_envelopes;
	std::vector< char > volume_types;
	std::vector< std::vector<float> > pitch_envelopes;
	std::vector< std::vector<float> > pitch_offset_envelopes;
	std::vector< std::vector<float> > dutycycle_envelopes;
	std::vector<float> prev_volumes; // volume before current volume envelope started
	std::vector<float> prev_duty;
	std::vector<int> prev_note_start;
	std::vector<int> note_end;
	std::vector<Wav_Start> wav_starts;
	std::vector<int> wav_sample;
	std::vector<bool> stretch_wavs;
	std::vector<Reverb_Type> reverb_types;
	char buf[1000];
	bool need_pe0 = false;

	int count = 0;
		
	int sz = (int)SDL_RWsize(f);

	while (count < sz && util::SDL_fgets(f, buf, 1000)) {
		count += strlen(buf);
		int pos = 0;
		std::string tok = token(buf, &pos);
		if (tok[0] >= 'A' && tok[0] <= 'Z') {
			int track = tok[0] - 'A';
			while ((int)tracks_s.size() < track+1) {
				tracks_s.push_back("");
				volumes.push_back(std::vector< std::pair<int, float> >());
				volumes[volumes.size()-1].push_back(std::pair<int, float>(0, 1.0f));
				volume_offsets.push_back(std::vector< std::pair<int, float> >());
				volume_offsets[volume_offsets.size()-1].push_back(std::pair<int, float>(0, 0.0f));
				dutycycles.push_back(std::vector< std::pair<int, float> >());
				dutycycles[dutycycles.size()-1].push_back(std::pair<int, float>(0, 128.0f/255.0f));
				pitches.push_back(std::vector<int>());
				pitch_offsets.push_back(std::vector<int>());
				note_lengths.push_back(4);
				tempos.push_back(120);
				octaves.push_back(4);
				sample.push_back(0);
				vol_start.push_back(0);
				vol_o_start.push_back(0);
				curr_vol.push_back(-1);
				curr_vol_o.push_back(-1);
				duty_start.push_back(0);
				curr_duty.push_back(-1);
				note.push_back(0);
				current_pitch.push_back(-1);
				current_pitch_o.push_back(-1);
				prev_ch.push_back('z');
				prev_volumes.push_back(1.0f);
				prev_duty.push_back(128.0f/255.0f);
				prev_note_start.push_back(0);
				note_end.push_back(0);
				wav_sample.push_back(-1);
				stretch_wavs.push_back(false);
			}
			while (isspace(buf[strlen(buf) - 1]))
				buf[strlen(buf) - 1] = 0;
			tracks_s[track] += std::string(buf + pos);
			tok = token(buf, &pos);
			while (tok[0]) {
				if (tok[0] == '@') {
					if (tok[1] == 'V') {
						if (tok[2] == 'A') {
							if (curr_vol[track] == -1) {
								vol_start[track] = sample[track];
								curr_vol[track] = atoi(tok.c_str() + 4);
								// not necessary to add volume here like with other envelopes
								// because they're added along with notes
							}
							else {
								int length = note_end[track] - prev_note_start[track];
								int sz = (int)volume_envelopes[curr_vol[track]].size();
								float stride = (float)length / sz;
								volumes[track].push_back(std::pair<int, float>(
									prev_note_start[track],
									prev_volumes[track]
								));
								float prev = prev_volumes[track];
								for (int i = 0; i < sz; i++) {
									float v = volume_envelopes[curr_vol[track]][i];
									int s;
									if (i == sz-1) {
										// this just makes it accurate
										s = note_end[track];
									}
									else {
										if (i < sz-1 && v == prev && v == volume_envelopes[curr_vol[track]][i+1]) {
											continue;
										}
										prev = v;
										s = int(prev_note_start[track] + stride * (i+1));
									}
									volumes[track].push_back(std::pair<int, float>(
										s,
										v
									));
								}
								volumes[track].push_back(std::pair<int, float>(
									note_end[track],
									prev_volumes[track]
								));
								curr_vol[track] = -1;
								prev_ch[track] = 'z'; // avoid closing volume at end of processing because we just did it (unless there's another note)
							}
						}
						else {
							if (curr_vol_o[track] == -1) {
								vol_o_start[track] = sample[track];
								curr_vol_o[track] = atoi(tok.c_str() + 3);
								volume_offsets[track].push_back(std::pair<int, float>(
									sample[track],
									0
								));
							}
							else {
								if ((int)volume_offset_envelopes.size() > curr_vol_o[track]) {
									int length = sample[track] - vol_o_start[track];
									int sz = (int)volume_offset_envelopes[curr_vol_o[track]].size();
									float stride = (float)length / sz;
									float prev = 0.0f;
									for (int i = 0; i < sz; i++) {
										float v = volume_offset_envelopes[curr_vol_o[track]][i];
										int s;
										if (i == sz-1) {
											// this just makes it accurate
											s = sample[track];
										}
										else {
											if (i < sz-1 && v == prev && v == volume_offset_envelopes[curr_vol_o[track]][i+1]) {
												continue;
											}
											prev = v;
											s = int(vol_o_start[track] + stride * (i+1));
										}
										volume_offsets[track].push_back(std::pair<int, float>(
											s,
											v
										));
									}
									volume_offsets[track].push_back(std::pair<int, float>(
										sample[track],
										0.0f
									));
								}
								curr_vol_o[track] = -1;
							}
						}
					}
					else if (tok[1] == 'P') {
						if (tok[2] == 'A' || tok[2] == 'N') {
							if (current_pitch[track] == -1) {
								current_pitch[track] = atoi(tok.c_str() + 3);
								if (current_pitch[track] >= (int)pitch_envelopes.size()) {
									current_pitch[track] = -1;
								}
							}
							else {
								current_pitch[track] = -1;
							}
						}
						else {
							if (current_pitch_o[track] == -1) {
								current_pitch_o[track] = atoi(tok.c_str() + 3);
								if (current_pitch_o[track] >= (int)pitch_offset_envelopes.size()) {
									current_pitch_o[track] = -1;
								}
							}
							else {
								current_pitch_o[track] = -1;
							}
						}
					}
					else if (tok[1] == 'D') {
						if (curr_duty[track] == -1) {
							duty_start[track] = sample[track];
							curr_duty[track] = atoi(tok.c_str() + 2);
						}
						else {
							if ((int)dutycycle_envelopes.size() > curr_duty[track]) {
								int length = sample[track] - duty_start[track];
								int sz = (int)dutycycle_envelopes[curr_duty[track]].size();
								float stride = length / sz;
								float prev = prev_duty[track];
								for (int i = 0; i < sz; i++) {
									float d = dutycycle_envelopes[curr_duty[track]][i];
									int s;
									if (i == sz-1) {
										// this just makes it accurate
										s = sample[track];
									}
									else {
										if (i < sz-1 && d == prev && d == dutycycle_envelopes[curr_duty[track]][i+1]) {
											continue;
										}
										prev = d;
										s = int(duty_start[track] + stride * (i+1));
									}
									dutycycles[track].push_back(std::pair<int, float>(
										s,
										d
									));
								}
								dutycycles[track].push_back(std::pair<int, float>(
									sample[track],
									prev_duty[track]
								));
							}
							curr_duty[track] = -1;
						}
					}
					else if (tok[1] == 'S') {
						int num = atoi(tok.c_str() + 2);
						if (num == wav_sample[track]) {
							wav_sample[track] = -1;
						}
						else {
							wav_sample[track] = num;
						}
					}
				}
				else {
					if ((tok[0] >= 'a' && tok[0] <= 'g') || tok[0] == 'r' || tok[0] == 'w') {
						if (need_pe0 && pitch_envelopes.size() > 0 && tok[0] >= 'a' && tok[0] <= 'g') {
							need_pe0 = false;

							int index = 0;

							index = indexes[tok[0]-'a'];
							if (tok[1] == '+') {
								index++;
							}
							else if (tok[1] == '-') {
								index--;
							}

							pitch_envelopes[pitch_envelopes.size()-1][0] = note_pitches[index][octaves[track]];
						}
						if (tok[0] != 'r') {
							prev_ch[track] = tok[0];
						}
						int save = sample[track];
						sample[track] += onenotelength(tok.c_str(), note_lengths[track], tempos[track], octaves[track], note[track], prev_ch[track]);
						note[track]++;
						if (tok[0] == 'w' || (tok[0] >= 'a' && tok[0] <= 'g')) {
							note_end[track] = sample[track];
						}
						if (tok[0] == 'r' || (tok[0] >= 'a' && tok[0] <= 'g')) {
							pitches[track].push_back(current_pitch[track]);
							pitch_offsets[track].push_back(current_pitch_o[track]);
							if (curr_vol[track] >= 0 && vol_start[track] < save) {
								// this is all operating on the previous note
								if (volume_types[curr_vol[track]] == 'S') {
									if ((int)volume_envelopes.size() > curr_vol[track]) {
										int length = save - prev_note_start[track];
										int sz = (int)volume_envelopes[curr_vol[track]].size();
										float stride = (float)length/sz;
										volumes[track].push_back(std::pair<int, float>(
											prev_note_start[track],
											prev_volumes[track]
										));
										float prev = prev_volumes[track];
										for (int i = 0; i < sz; i++) {
											float v = volume_envelopes[curr_vol[track]][i];
											int s;
											if (i == sz-1) {
												// this just makes it accurate
												s = save;
											}
											else {
												if (v == prev && v == volume_envelopes[curr_vol[track]][i+1]) {
													continue;
												}
												prev = v;
												s = int(prev_note_start[track] + stride * (i+1));
											}
											volumes[track].push_back(std::pair<int, float>(
												s,
												v
											));
										}
										volumes[track].push_back(std::pair<int, float>(
											save,
											prev_volumes[track]
										));
									}
								}
							}
							if (wav_sample[track] >= 0 && (tok[0] >= 'a' && tok[0] <= 'g')) {
								if (wav_starts.size() == 0 || wav_starts[wav_starts.size()-1].sample != wav_sample[track] || wav_starts[wav_starts.size()-1].play_start != (Uint32)save) { // avoid duplicates
									Wav_Start w;
									w.sample = wav_sample[track];
									w.play_start = save;
									char ch = tok[0];
									int total = onenotelength(tok.c_str(), note_lengths[track], tempos[track], octaves[track], note[track], 'z'); // z == nothing never used
									int pos2 = pos;
									std::string tok2;
									do {
										tok2 = token(buf, &pos2);
										if (tok2[0] == 'w') {
											total += onenotelength(tok2.c_str(), note_lengths[track], tempos[track], octaves[track], note[track], ch);
										}
									} while (tok2[0] == 'w');
									w.length = stretch_wavs[track] ? total : 0;
									//w.length = stretch_wavs[track] ? sample[track] - save : 0;
									w.instance = 0;
									w.volume = prev_volumes[track];
									wav_starts.push_back(w);
								}
							}
							//if (tok[0] != 'r') {
								prev_note_start[track] = save;
							//}
						}
					}
					else if (tok[0] == 'v') {
						volumes[track].push_back(std::pair<int, float>(sample[track], prev_volumes[track]));
						float vol = atoi(tok.c_str() + 1) / 255.0f;
						prev_volumes[track] = vol;
						volumes[track].push_back(std::pair<int, float>(sample[track], vol));
					}
					else if (tok[0] == 'y') {
						dutycycles[track].push_back(std::pair<int, float>(
							sample[track],
							prev_duty[track]
						));
						float duty = atoi(tok.c_str() + 1) / 255.0f;
						prev_duty[track] = duty;
						dutycycles[track].push_back(std::pair<int, float>(
							sample[track],
							duty
						));
					}
					else if (tok[0] == 'l') {
						note_lengths[track] = atoi(tok.c_str() + 1);
					}
					else if (tok[0] == 't') {
						tempos[track] = atoi(tok.c_str() + 1);
					}
					else if (tok[0] == 'o') {
						octaves[track] = atoi(tok.c_str() + 1);
					}
					else if (tok[0] == '>') {
						octaves[track]--;
						if (octaves[track] < 0) {
							octaves[track] = 0;
						}
					}
					else if (tok[0] == '<') {
						octaves[track]++;
						if (octaves[track] > 11) {
							octaves[track] = 11;
						}
					}
					else if (tok[0] == 's') {
						int i = atoi(tok.c_str() + 1);
						stretch_wavs[track] = i != 0;
					}
				}
				tok = token(buf, &pos);
			}
		}
		else if (tok[0] == '@') {
			if (tok[1] == 'V') {
				if (tok[2] == 'A') {
					char type = tok[3];
					int num = atoi(tok.c_str() + 4); // 4 because 3 is 'S' or 'M' for single or multi
					std::vector<float> envelope;
					while (tok[0] != 0) {
						tok = token(buf, &pos);
						if (isdigit(tok[0])) {
							envelope.push_back(atoi(tok.c_str())/255.0f);
						}
					}
					assert(envelope.size() > 0 && "Volume envelope with no data");
					while ((int)volume_envelopes.size() <= num) {
						volume_envelopes.push_back(std::vector<float>());
						volume_types.push_back(' ');
					}
					volume_envelopes[num] = envelope;
					volume_types[num] = type;
				}
				else { // 'O'
					int num = atoi(tok.c_str() + 3);
					std::vector<float> envelope;
					while (tok[0] != 0) {
						tok = token(buf, &pos);
						if (tok[0] == '-' || isdigit(tok[0])) {
							envelope.push_back(atoi(tok.c_str())/255.0f);
						}
					}
					assert(envelope.size() > 0 && "Volume envelope with no data");
					while ((int)volume_offset_envelopes.size() <= num) {
						volume_offset_envelopes.push_back(std::vector<float>());
					}
					volume_offset_envelopes[num] = envelope;
				}
			}
			else if (tok[1] == 'P') {
				if (tok[2] == 'A') { // PA# pitch to absolute frequency
					int num = atoi(tok.c_str() + 3);
					std::vector<float> envelope;
					envelope.push_back(0.0f); // PA# and PN# need help - insert a zero to be filled with the next note
					need_pe0 = true;
					while (tok[0] != 0) {
						tok = token(buf, &pos);
						if (isdigit(tok[0])) {
							envelope.push_back((float)atoi(tok.c_str()));
						}
					}
					while ((int)pitch_envelopes.size() <= num) {
						pitch_envelopes.push_back(std::vector<float>());
					}
					pitch_envelopes[num] = envelope;
				}
				else if (tok[2] == 'N') { // PN# pitch to note
					int num = atoi(tok.c_str() + 3);
					std::vector<float> envelope;
					envelope.push_back(0.0f); // PA# and PN# need help - insert a zero to be filled with the next note
					need_pe0 = true;
					while (tok[0] != 0) {
						tok = token(buf, &pos);
						if (tok[0] >= 'a' && tok[0] <= 'g') {
							int index = indexes[tok[0] - 'a'];
							int offset = 1;
							if (tok[1] == '+') {
								index++;
								offset++;
							}
							else if (tok[1] == '-') {
								index--;
								offset++;
							}
							int octave = atoi(tok.c_str() + offset);
							if (octave < 0) {
								octave = 0;
							}
							else if (octave > 11) {
								octave = 11;
							}
							envelope.push_back(note_pitches[index][octave]);
						}
					}
					while ((int)pitch_envelopes.size() <= num) {
						pitch_envelopes.push_back(std::vector<float>());
					}
					pitch_envelopes[num] = envelope;
				}
				else { // PO# pitch offset by frequency
					int num = atoi(tok.c_str() + 3);
					std::vector<float> envelope;
					while (tok[0] != 0) {
						tok = token(buf, &pos);
						if (tok[0] == '-' || isdigit(tok[0])) {
							envelope.push_back((float)atoi(tok.c_str()));
						}
					}
					while ((int)pitch_offset_envelopes.size() <= num) {
						pitch_offset_envelopes.push_back(std::vector<float>());
					}
					pitch_offset_envelopes[num] = envelope;
				}
			}
			else if (tok[1] == 'D') {
				int num = atoi(tok.c_str() + 2);
				std::vector<float> envelope;
				while (tok[0] != 0) {
					tok = token(buf, &pos);
					if (isdigit(tok[0])) {
						envelope.push_back(atoi(tok.c_str())/255.0f);
					}
				}
				while ((int)dutycycle_envelopes.size() <= num) {
					dutycycle_envelopes.push_back(std::vector<float>());
				}
				dutycycle_envelopes[num] = envelope;
			}
			else if (tok[1] == 'S') {
				int num = atoi(tok.c_str() + 2);
				std::string sample_name;
				while (buf[pos] && buf[pos] != '{') {
					pos++;
				}
				if (buf[pos] == '{') {
					pos++;
					while (buf[pos] && buf[pos] != '}') {
						char s[2];
						s[0] = buf[pos];
						s[1] = 0;
						sample_name += std::string(s);
						pos++;
					}
					if (buf[pos] == '}') {
						pos++;
						util::trim(sample_name);
						Sample *sample = new Sample(sample_name, load_from_filesystem);
						while ((int)wav_samples.size() <= num) {
							wav_samples.push_back(0);
						}
						wav_samples[num] = sample;
					}
				}
			}
			else if (tok[1] == 'R') {
				int num = atoi(tok.c_str() + 2);
				std::string s = buf;
				size_t pos = s.find('{');
				std::string values = s.substr(pos+1);
				util::trim(values);
				util::Tokenizer t(values, ' ');
				std::string reverberations_s = t.next();
				std::string falloff_interpolator_s = t.next();
				std::string falloff_time_s = t.next();
				std::string start_volume_s = t.next();
				std::string final_volume_s = t.next();
				Reverb_Type rt;
				memset(&rt, 0, sizeof(rt));
				while ((int)reverb_types.size() <= num) {
					reverb_types.push_back(rt);
				}
				rt.reverberations = atoi(reverberations_s.c_str());
				rt.falloff_interpolator = atoi(falloff_interpolator_s.c_str());
				rt.falloff_time = atoi(falloff_time_s.c_str());
				rt.start_volume = atoi(start_volume_s.c_str());
				rt.final_volume = atoi(final_volume_s.c_str());
				reverb_types[num] = rt;
			}
		}
	}

	int longest = 0;

	for (size_t i = 0; i < tracks_s.size(); i++) {
		// count longest track
		if (sample[i] > longest) {
			longest = sample[i];
		}

		// Volumes and duty cycles must have an explicit ending.
		volumes[i].push_back(std::pair<int, float>(
			sample[i],
			prev_volumes[i]
		));
		volume_offsets[i].push_back(std::pair<int, float>(
			sample[i],
			0.0f
		));
		dutycycles[i].push_back(std::pair<int, float>(
			sample[i],
			prev_duty[i]
		));
	}

	remove_dups(volumes);
	remove_dups(volume_offsets);
	remove_dups(dutycycles);

	for (size_t i = 0; i < tracks_s.size(); i++) {
		std::vector<Wav_Start> w; // dummy
		tracks.push_back(new Track(MML::PULSE, tracks_s[i], volumes[i], volume_offsets[i], pitches[i], pitch_offsets[i], pitch_envelopes, pitch_offset_envelopes, dutycycles[i], longest-sample[i], wav_samples, i == 0 ? wav_starts : w, 0, this, reverb_types));
	}

	num_tracks = (int)tracks.size();

	if (load_from_filesystem) {
		SDL_RWclose(f);
	}
	else {
		util::close_file(f);
	}

	SDL_LockMutex(internal::audio_context.mixer_mutex);

	loaded_mml.push_back(this);

	SDL_UnlockMutex(internal::audio_context.mixer_mutex);

#ifdef DUMP
	if (dumpfile == 0) {
		dumpfile = SDL_RWFromFile("dump.raw", "wb");
	}
#endif
	int arg;
	if ((arg = util::check_args(shim::argc, shim::argv, "+mml_loops")) > 0) {
		mml_loops = atoi(shim::argv[arg+1]);
		use_mml_loops = true;
	}
	mml_loops *= num_tracks;
}

MML::~MML()
{
	SDL_LockMutex(internal::audio_context.mixer_mutex);

	for (size_t i = 0; i < loaded_mml.size(); i++) {
		if (loaded_mml[i] == this) {
			loaded_mml.erase(loaded_mml.begin()+i);
			break;
		}
	}

	SDL_UnlockMutex(internal::audio_context.mixer_mutex);

	for (size_t i =  0; i < tracks.size(); i++) {
		delete tracks[i];
	}
	
	for (size_t i =  0; i < reverb_tracks.size(); i++) {
		delete reverb_tracks[i];
	}
	
	for (size_t i = 0; i < wav_samples.size(); i++) {
		delete wav_samples[i];
	}

#ifdef DUMP
	SDL_RWclose(dumpfile);
#endif
}

void MML::play(float volume, bool loop)
{
	if (internal::audio_context.mute) {
		return;
	}

	set_master_volume(volume);

	if (this == shim::music) {
		set_pause_with_sfx(false);
	}

	SDL_LockMutex(internal::audio_context.mixer_mutex);

	for (size_t i = 0; i < tracks.size(); i++) {
		tracks[i]->play(loop);
	}

	for (size_t i = 0; i < reverb_tracks.size(); i++) {
		reverb_tracks[i]->play(false); // never loop!
	}

	SDL_UnlockMutex(internal::audio_context.mixer_mutex);
}

void MML::play(bool loop)
{
	play(shim::sfx_volume, loop);
}

void MML::pause()
{
	SDL_LockMutex(internal::audio_context.mixer_mutex);

	for (size_t i = 0; i < tracks.size(); i++) {
		tracks[i]->pause();
	}

	for (size_t i = 0; i < reverb_tracks.size(); i++) {
		reverb_tracks[i]->pause();
	}

	SDL_UnlockMutex(internal::audio_context.mixer_mutex);
}

void MML::stop()
{
	SDL_LockMutex(internal::audio_context.mixer_mutex);

	for (size_t i = 0; i < tracks.size(); i++) {
		tracks[i]->stop();
	}
	
	for (size_t i = 0; i < reverb_tracks.size(); i++) {
		delete reverb_tracks[i];
	}
	reverb_tracks.clear();

	SDL_UnlockMutex(internal::audio_context.mixer_mutex);
}

std::string MML::get_name()
{
	return name;
}

bool MML::is_done()
{
	for (size_t i = 0; i < tracks.size(); i++) {
		if (tracks[i]->is_done() == false) {
			return false;
		}
	}

	return true;
}

void MML::set_master_volume(float master_volume)
{
	for (size_t i = 0; i < tracks.size(); i++) {
		tracks[i]->set_master_volume(master_volume/tracks.size(), master_volume);
	}
	/*
	for (size_t i = 0; i < reverb_tracks.size(); i++) {
		reverb_tracks[i]->set_master_volume(master_volume/reverb_tracks.size(), master_volume);
	}
	*/
}

float MML::get_master_volume()
{
	if (tracks.size() > 0) {
		return tracks[0]->get_master_volume();
	}
	else {
		return 1.0f;
	}
}

void MML::set_pause_with_sfx(bool pause_with_sfx)
{
	_pause_with_sfx = pause_with_sfx;
	for (size_t i = 0; i < tracks.size(); i++) {
		tracks[i]->set_pause_with_sfx(pause_with_sfx);
	}
	for (size_t i = 0; i < reverb_tracks.size(); i++) {
		reverb_tracks[i]->set_pause_with_sfx(pause_with_sfx);
	}
}

bool MML::is_playing()
{
	bool playing = false;
	for (size_t i = 0; i < tracks.size(); i++) {
		if (tracks[i]->is_playing()) {
			playing = true;
			break;
		}
	}
	return playing;
}

void play_music(std::string name)
{
	if (shim::music && shim::music->get_name() == name) {
		if (shim::music->is_playing() == false) {
			shim::music->set_pause_with_sfx(false);
			shim::music->play(shim::music_volume, true);
		}
	}
	else {
		delete shim::music;
		shim::music = new MML(name);
		shim::music->set_pause_with_sfx(false);
		shim::music->play(shim::music_volume, true);
	}
}

void pause_music()
{
	if (shim::music) {
		shim::music->pause();
	}
}

void stop_music()
{
	if (shim::music) {
		shim::music->stop();
	}
}

} // End namespace audio

} // End namespace noo
