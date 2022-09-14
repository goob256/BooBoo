#ifndef NOO_SOUND_H
#define NOO_SOUND_H

// Base interface that works with MML/Sample

#include "shim4/main.h"

namespace noo {

namespace audio {

class SHIM4_EXPORT Sound {
public:
	virtual void play(float volume, bool loop) = 0;
	virtual void play(bool loop) = 0;
	virtual bool is_done() = 0;
	virtual void stop() = 0;
	virtual ~Sound();
};

} // End namespace audio

} // End namespace noo

#endif // NOO_SOUND_H
