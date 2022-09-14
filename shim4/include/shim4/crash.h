#ifndef NOO_CRASH_H
#define NOO_CRASH_H

#include "shim4/main.h"

using namespace noo;

namespace noo {

namespace util {

void SHIM4_EXPORT start_crashdumps();
void SHIM4_EXPORT end_crashdumps();

} // End namespace util

} // End namespace noo

#endif // NOO_CRASH_H
