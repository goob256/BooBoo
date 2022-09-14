#ifndef NOO_I_SHIM_H
#define NOO_I_SHIM_H

#include "shim4/main.h"

namespace noo {

namespace shim {

namespace internal {

TGUI_Event *handle_tgui_event(TGUI_Event *sdl_event);

} // End namespace internal

} // End namespace shim

} // End namespace noo

#endif // NOO_I_SHIM_H
