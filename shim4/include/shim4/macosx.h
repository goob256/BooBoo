#ifndef NOO_MACOSX_H
#define NOO_MACOSX_H

#include "shim4/gui.h"
#include "shim4/util.h"

namespace noo {

namespace gfx {

void macosx_centre_window(void *window);
util::Size<int> macosx_get_desktop_resolution();
void macosx_set_background_colour(void *window, SDL_Colour colour);

} // End namespace gfx

namespace gui {

int macosx_popup(std::string caption, std::string title, gui::Popup_Type type);

} // End namespace gui

namespace util {

void macosx_log(const char *s);
std::string macosx_get_standard_path(Path_Type type);
void macosx_open_with_system(std::string filename);
std::string macosx_get_system_language();

} // End namespace util

} // End namespace noo

#endif // NOO_MACOSX_H
