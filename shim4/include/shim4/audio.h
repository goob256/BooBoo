#ifndef NOO_AUDIO_H
#define NOO_AUDIO_H

#include "shim4/main.h"

namespace noo {

namespace audio {

bool SHIM4_EXPORT static_start();
bool SHIM4_EXPORT start();
void SHIM4_EXPORT end();
int SHIM4_EXPORT millis_to_samples(int millis);
int SHIM4_EXPORT samples_to_millis(int samples, int freq = -1);
void SHIM4_EXPORT pause_sfx(bool paused);

} // End namespace audio

} // End namespace noo

#endif // NOO_AUDIO_H
