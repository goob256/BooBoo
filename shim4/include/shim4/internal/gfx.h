#ifndef NOO_I_GFX_H
#define NOO_I_GFX_H

#include "shim4/main.h"

namespace noo {

namespace gfx {

class Image;
class Shader;

namespace internal {

struct GFX_Context {
	bool inited;
	bool fullscreen;
	bool fullscreen_window;
	Image *target_image;
	Image *work_image; // same size as real screen size
	Image *plasma;
	SDL_Window *window;
	Uint32 windowid;
	bool restarting;
	SDL_GLContext opengl_context;
	SDL_mutex *draw_mutex;
	bool mouse_in_window;
	Shader *textured_shader;
	Shader *untextured_shader;
#ifdef _WIN32
	HWND hwnd;
	bool d3d_lost;
	LPDIRECT3DSURFACE9 render_target;
	LPDIRECT3DSURFACE9 depth_stencil_buffer;
#elif defined __linux__ && !defined ANDROID
	Display *x_display;
	Window x_window;
#elif defined IOS
	GLuint framebuffer; // must be bound when drawing to screen
	GLuint colorbuffer; // must be bound when flipping
#endif
};

extern GFX_Context gfx_context;

bool scale_mouse_event(TGUI_Event *event);
void handle_lost_device(bool including_opengl, bool force = false);
void handle_found_device(bool including_opengl, bool force = false);
int My_SDL_GetCurrentDisplayMode(int adapter, SDL_DisplayMode *mode);
void recreate_work_image();

#if defined _WIN32
HICON win_create_icon(HWND wnd, Uint8 *data, util::Size<int> size, int xfocus, int yfocus, bool is_cursor);
#elif defined __linux__ && !defined ANDROID
Cursor x_create_cursor(Display *display, Uint8 *data, util::Size<int> size, int xfocus, int yfocus);
#endif

} // End namespace internal

} // End namespace gfx

} // End namespace noo

#endif // NOO_I_GFX_H
