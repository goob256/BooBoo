#ifndef NOO_FLAC_H
#define NOO_FLAC_H

#include "shim4/main.h"

namespace noo {

namespace audio {

Uint8 *decode_flac(SDL_RWops *file, char *errmsg, SDL_AudioSpec *spec); // Returns 0 on error, fills errmsg

} // End namespace audio

} // End namespace noo

#endif // NOO_FLAC_H
