#ifndef NOO_I_AUDIO_H
#define NOO_I_AUDIO_H

namespace noo {

namespace audio {

namespace internal {

struct Audio_Context {
	bool mute;
	SDL_mutex *mixer_mutex;
	SDL_AudioSpec device_spec;
	std::vector<Sample_Instance *> playing_samples;
};

extern Audio_Context audio_context;

} // End namespace internal

} // End namespace audio

} // End namespace noo

#endif // NOO_I_AUDIO_H
