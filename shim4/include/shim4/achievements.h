#ifndef NOO_ACHIEVEMENTS_H
#define NOO_ACHIEVEMENTS_H

#include "shim4/main.h"

namespace noo {

namespace util {

bool SHIM4_EXPORT achieve(void *id);
bool SHIM4_EXPORT show_achievements();

// FIXME: stuff for setting up achievements (store system-specific ids/etc)

} // End namespace util

} // End namespace noo

#endif // NOO_ACHIEVEMENTS_H
