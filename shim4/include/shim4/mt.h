#ifndef NOO_MT_H
#define NOO_MT_H

#include "shim4/main.h"

using namespace noo;

namespace noo {

namespace util {

void SHIM4_EXPORT srand(uint32_t s);
uint32_t SHIM4_EXPORT rand();
uint32_t SHIM4_EXPORT rand(int min, int max_inclusive);

} // End namespace util

} // End namespace noo

#endif // NOO_MT_H
