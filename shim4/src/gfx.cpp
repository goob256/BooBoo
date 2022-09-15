#include "shim4/error.h"
#include "shim4/gfx.h"
#include "shim4/gui.h"
#include "shim4/image.h"
#include "shim4/json.h"
#include "shim4/model.h"
#include "shim4/mt.h"
#include "shim4/primitives.h"
#include "shim4/shader.h"
#include "shim4/shim.h"
#include "shim4/sprite.h"
#include "shim4/tilemap.h"
#include "shim4/ttf.h"
#include "shim4/utf8.h"
#include "shim4/util.h"
#include "shim4/vertex_cache.h"

#include "shim4/internal/gfx.h"
#include "shim4/internal/shim.h"

#ifdef _WIN32
#define NOOSKEWL_SHIM_DEFAULT_FVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE4(1))
#endif

#ifdef _WIN32
#define strdup _strdup
#endif

using namespace noo;

#if !defined ANDROID && !defined IOS
glStencilFuncSeparate_func glStencilFuncSeparate_ptr;
glStencilOpSeparate_func glStencilOpSeparate_ptr;
glBindFramebuffer_func glBindFramebuffer_ptr;
glDeleteRenderbuffers_func glDeleteRenderbuffers_ptr;
glGenFramebuffers_func glGenFramebuffers_ptr;
glGenRenderbuffers_func glGenRenderbuffers_ptr;
glBindRenderbuffer_func glBindRenderbuffer_ptr;
glFramebufferTexture2D_func glFramebufferTexture2D_ptr;
glRenderbufferStorage_func glRenderbufferStorage_ptr;
glCheckFramebufferStatus_func glCheckFramebufferStatus_ptr;
glDeleteFramebuffers_func glDeleteFramebuffers_ptr;
glFramebufferRenderbuffer_func glFramebufferRenderbuffer_ptr;
glUseProgram_func glUseProgram_ptr;
glUniform1f_func glUniform1f_ptr;
glUniform2f_func glUniform2f_ptr;
glUniform3f_func glUniform3f_ptr;
glUniform4f_func glUniform4f_ptr;
glUniform1i_func glUniform1i_ptr;
glUniform2i_func glUniform2i_ptr;
glUniform3i_func glUniform3i_ptr;
glUniform4i_func glUniform4i_ptr;
glUniform1fv_func glUniform1fv_ptr;
glUniform2fv_func glUniform2fv_ptr;
glUniform3fv_func glUniform3fv_ptr;
glUniform4fv_func glUniform4fv_ptr;
glUniform1iv_func glUniform1iv_ptr;
glUniform2iv_func glUniform2iv_ptr;
glUniform3iv_func glUniform3iv_ptr;
glUniform4iv_func glUniform4iv_ptr;
glUniformMatrix2fv_func glUniformMatrix2fv_ptr;
glUniformMatrix3fv_func glUniformMatrix3fv_ptr;
glUniformMatrix4fv_func glUniformMatrix4fv_ptr;
glDeleteShader_func glDeleteShader_ptr;
glCreateProgram_func glCreateProgram_ptr;
glDeleteProgram_func glDeleteProgram_ptr;
glAttachShader_func glAttachShader_ptr;
glLinkProgram_func glLinkProgram_ptr;
glGetAttribLocation_func glGetAttribLocation_ptr;
glGetTexImage_func glGetTexImage_ptr;
glEnableVertexAttribArray_func glEnableVertexAttribArray_ptr;
glVertexAttribPointer_func glVertexAttribPointer_ptr;
glGetUniformLocation_func glGetUniformLocation_ptr;
glShaderSource_func glShaderSource_ptr;
glCompileShader_func glCompileShader_ptr;
glGetShaderiv_func glGetShaderiv_ptr;
glGetShaderInfoLog_func glGetShaderInfoLog_ptr;
glCreateShader_func glCreateShader_ptr;
glBlendFunc_func glBlendFunc_ptr;
glEnable_func glEnable_ptr;
glDisable_func glDisable_ptr;
glFrontFace_func glFrontFace_ptr;
glCullFace_func glCullFace_ptr;
glScissor_func glScissor_ptr;
glViewport_func glViewport_ptr;
glClearColor_func glClearColor_ptr;
glClear_func glClear_ptr;
glClearDepthf_func glClearDepthf_ptr;
#if !defined ANDROID && !defined IOS
glClearDepth_func glClearDepth_ptr;
#endif
glClearStencil_func glClearStencil_ptr;
glDepthMask_func glDepthMask_ptr;
glDepthFunc_func glDepthFunc_ptr;
glStencilFunc_func glStencilFunc_ptr;
glStencilOp_func glStencilOp_ptr;
glActiveTexture_func glActiveTexture_ptr;
glColorMask_func glColorMask_ptr;
glDeleteTextures_func glDeleteTextures_ptr;
glGenTextures_func glGenTextures_ptr;
glBindTexture_func glBindTexture_ptr;
glTexImage2D_func glTexImage2D_ptr;
glTexParameteri_func glTexParameteri_ptr;
glGetError_func glGetError_ptr;
glDrawArrays_func glDrawArrays_ptr;
#endif

#if defined __APPLE__ && !defined IOS
#include "shim4/macosx.h"
#endif

#if defined __linux__ && !defined ANDROID
#include "shim4/x.h"
#endif

#include "shim4/shaders/glsl/default_vertex.h"
#include "shim4/shaders/glsl/default_fragment.h"
#include "shim4/shaders/glsl/default_textured_fragment.h"
#include "shim4/shaders/glsl/model_vertex.h"
#include "shim4/shaders/glsl/model_fragment.h"
#include "shim4/shaders/glsl/appear_fragment.h"

#ifdef _WIN32
#include "shim4/shaders/hlsl/default_vertex.h"
#include "shim4/shaders/hlsl/default_fragment.h"
#include "shim4/shaders/hlsl/default_textured_fragment.h"
#include "shim4/shaders/hlsl/model_fragment.h"
#endif

static int scaled_w;
static int scaled_h;
static int total_frames;
static Uint32 fps_start;
static int fps;
static bool show_fps;
static bool vsync;
static glm::mat4 modelview;
static glm::mat4 proj;
static Uint32 fancy_draw_start;
static std::vector<std::string> notifications;
static Uint32 notification_start_time;
static int scissor_x;
static int scissor_y;
static int scissor_w;
static int scissor_h;
static bool force_integer_scaling;
static gfx::_lost_device_callback lost_device_callback;
static gfx::_lost_device_callback found_device_callback;
static bool two_sided_stencil;
static float min_aspect_ratio;
static float max_aspect_ratio;
static bool mouse_cursor_shown;
static bool depth_write_enabled;
static bool create_depth_buffer;
static bool create_stencil_buffer;
static bool blending_enabled;
static util::Size<int> minimum_window_size;
static util::Size<int> maximum_window_size;
static int press_and_hold_state;
static util::Size<int> last_gui_size;
static int d3d_device_count;
static int d3d_count;
static int d3d_device_depth_count;
static util::Size<int> last_screen_mode;
static bool handled_lost;
static float screen_shake_amount;
static Uint32 screen_shake_end;
static glm::mat4 screen_shake_mv;
static glm::mat4 screen_shake_p;
static glm::mat4 default_modelview;
static glm::mat4 default_proj;

#if defined _WIN32
static D3DPRESENT_PARAMETERS d3d_pp;
static IDirect3D9 *d3d;
static HICON icon_small, icon_big;
#endif

#if ((defined __APPLE__ && !defined IOS) || (defined __linux__ && !defined ANDROID && !defined RASPBERRYPI) || defined _WIN32)
static bool use_custom_cursor;
SDL_Surface *mouse_cursor_surface;
SDL_Cursor *mouse_cursor;
#endif

static bool curr_font_size;

static void next_notification()
{
	notifications.erase(notifications.begin());
	notification_start_time = SDL_GetTicks();
}

namespace noo {

namespace gfx {

static void audit()
{
	if (shim::opengl == false) {
		util::debugmsg("d3d_count=%d\n", d3d_count);
		util::debugmsg("d3d_device_count=%d\n", d3d_device_count);
		util::debugmsg("d3d_device_depth_count=%d\n", d3d_device_depth_count);
	}
}

static void set_opengl()
{
#ifdef _WIN32
	shim::opengl = util::bool_arg(false, shim::argc, shim::argv, "opengl");
#else
	shim::opengl = true;
#endif
}

#ifdef _WIN32
static int shim_compare_to_d3d(Compare_Func func)
{
	switch (func) {
		case COMPARE_NEVER:
			return D3DCMP_NEVER;
		case COMPARE_LESS:
			return D3DCMP_LESS;
		case COMPARE_EQUAL:
			return D3DCMP_EQUAL;
		case COMPARE_LESSEQUAL:
			return D3DCMP_LESSEQUAL;
		case COMPARE_GREATER:
			return D3DCMP_GREATER;
		case COMPARE_NOTEQUAL:
			return D3DCMP_NOTEQUAL;
		case COMPARE_GREATEREQUAL:
			return D3DCMP_GREATEREQUAL;
		case COMPARE_ALWAYS:
			return D3DCMP_ALWAYS;
	}

	return -1;
}

static int shim_stencilop_to_d3d(Stencil_Op op)
{
	switch (op) {
		case STENCILOP_KEEP:
			return D3DSTENCILOP_KEEP;
		case STENCILOP_ZERO:
			return D3DSTENCILOP_ZERO;
		case STENCILOP_REPLACE:
			return D3DSTENCILOP_REPLACE;
		case STENCILOP_INCRSAT:
			return D3DSTENCILOP_INCRSAT;
		case STENCILOP_DECRSAT:
			return D3DSTENCILOP_DECRSAT;
		case STENCILOP_INVERT:
			return D3DSTENCILOP_INVERT;
		case STENCILOP_INCR:
			return D3DSTENCILOP_INCR;
		case STENCILOP_DECR:
			return D3DSTENCILOP_DECR;
	}

	return -1;
}

static int shim_blend_to_d3d(Blend_Mode m)
{
	if (m == BLEND_ZERO) {
		return D3DBLEND_ZERO;
	}
	else if (m == BLEND_ONE) {
		return D3DBLEND_ONE;
	}
	else if (m == BLEND_SRCCOLOR) {
		return D3DBLEND_SRCCOLOR;
	}
	else if (m == BLEND_INVSRCCOLOR) {
		return D3DBLEND_INVSRCCOLOR;
	}
	else if (m == BLEND_SRCALPHA) {
		return D3DBLEND_SRCALPHA;
	}
	else if (m == BLEND_INVSRCALPHA) {
		return D3DBLEND_INVSRCALPHA;
	}

	return -1;
}

static void d3d_create_depth_buffer()
{
	if (::create_depth_buffer) {
		if (internal::gfx_context.depth_stencil_buffer == 0) {
			util::Size<int> size;
			if (shim::depth_buffer_size.w < 0 || shim::depth_buffer_size.h < 0) {
				size = shim::real_screen_size;
			}
			else {
				size = shim::depth_buffer_size;
			}

			D3DFORMAT format;
			if (::create_stencil_buffer) {
				format = D3DFMT_D24S8;
			}
			else {
				format = D3DFMT_D16;
			}
			if (shim::d3d_device->CreateDepthStencilSurface(size.w, size.h, format, D3DMULTISAMPLE_NONE, 0, true, &internal::gfx_context.depth_stencil_buffer, 0) != D3D_OK) {
				throw util::Error("CreateDepthStencilSurface failed");
			}
			else {
				d3d_device_depth_count++;
			}

			shim::d3d_device->SetDepthStencilSurface(internal::gfx_context.depth_stencil_buffer);
		}
	}
	else {
		internal::gfx_context.depth_stencil_buffer = 0;
	}
}

static void set_initial_d3d_state()
{
	shim::d3d_device->BeginScene();

	shim::d3d_device->SetRenderState(D3DRS_LIGHTING, FALSE);
	shim::d3d_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	shim::d3d_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	shim::d3d_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	shim::d3d_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	shim::d3d_device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	shim::d3d_device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	shim::d3d_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	shim::d3d_device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

	if (shim::d3d_device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP) != D3D_OK) {
		util::infomsg("SetSamplerState failed.\n");
	}
	if (shim::d3d_device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP) != D3D_OK) {
		util::infomsg("SetSamplerState failed.\n");
	}
	if (shim::d3d_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT) != D3D_OK) {
		util::infomsg("SetSamplerState failed.\n");
	}
	if (shim::d3d_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT) != D3D_OK) {
		util::infomsg("SetSamplerState failed.\n");
	}

	shim::d3d_device->SetFVF(NOOSKEWL_SHIM_DEFAULT_FVF);
}

static void fill_d3d_pp(int w, int h)
{
	ZeroMemory(&d3d_pp, sizeof(d3d_pp));

	d3d_pp.BackBufferWidth = w;
	d3d_pp.BackBufferHeight = h;
	d3d_pp.BackBufferCount = 1;
	d3d_pp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3d_pp.hDeviceWindow = internal::gfx_context.hwnd;
	d3d_pp.Windowed = internal::gfx_context.fullscreen ? 0 : 1;
	d3d_pp.EnableAutoDepthStencil = FALSE;
	d3d_pp.Flags = 0;

	D3DFORMAT format = D3DFMT_X8R8G8B8;

	if (internal::gfx_context.fullscreen) {
		int num_modes = d3d->GetAdapterModeCount(shim::adapter, D3DFMT_X8R8G8B8);
		shim::refresh_rate = 0;
		for (int i = 0; i < num_modes; i++) {
			D3DDISPLAYMODE mode;
			d3d->EnumAdapterModes(shim::adapter, D3DFMT_X8R8G8B8, i, &mode);
			if (mode.Width == w && mode.Height == h && (int)mode.RefreshRate > shim::refresh_rate) {
				shim::refresh_rate = mode.RefreshRate;
				format = mode.Format;
			}
		}
	}
	else {
		format = D3DFMT_UNKNOWN;
	}
	d3d_pp.BackBufferFormat = format;
	d3d_pp.FullScreen_RefreshRateInHz = internal::gfx_context.fullscreen ? shim::refresh_rate : 0;

	if (vsync) {
		d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	}
	else {
		d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	}
}

static void create_d3d_device()
{
	internal::gfx_context.d3d_lost = false;

	HRESULT hr;
	// NOTE: My desktop PC with NVIDIA graphics fails at screen rotation unless I use software vertex processing
	if ((hr = d3d->CreateDevice(shim::adapter, D3DDEVTYPE_HAL, internal::gfx_context.hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3d_pp, (LPDIRECT3DDEVICE9 *)&shim::d3d_device)) != D3D_OK) {
		if ((hr = d3d->CreateDevice(shim::adapter, D3DDEVTYPE_HAL, internal::gfx_context.hwnd, D3DCREATE_MIXED_VERTEXPROCESSING, &d3d_pp, (LPDIRECT3DDEVICE9 *)&shim::d3d_device)) != D3D_OK) {
			if ((hr = d3d->CreateDevice(shim::adapter, D3DDEVTYPE_HAL, internal::gfx_context.hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3d_pp, (LPDIRECT3DDEVICE9 *)&shim::d3d_device)) != D3D_OK) {
				throw util::Error("Unable to create D3D device");
			}
			else {
				d3d_device_count++;
			}
		}
		else {
			d3d_device_count++;
		}
	}
	else {
		d3d_device_count++;
	}
}

static void paint_window_black(int w, int h)
{
	// This clears the screen to black right away to avoid a white flash on Windows
	PAINTSTRUCT ps;
	HDC hdc;

	hdc = BeginPaint(internal::gfx_context.hwnd, &ps);

	SelectObject(hdc, GetStockObject(DC_BRUSH));
	SetDCBrushColor(hdc, RGB(shim::black.r, shim::black.g, shim::black.b));

	Rectangle(hdc, 0, 0, w, h);
}
#endif

static int shim_compare_to_gl(Compare_Func func)
{
	switch (func) {
		case COMPARE_NEVER:
			return GL_NEVER;
		case COMPARE_LESS:
			return GL_LESS;
		case COMPARE_EQUAL:
			return GL_EQUAL;
		case COMPARE_LESSEQUAL:
			return GL_LEQUAL;
		case COMPARE_GREATER:
			return GL_GREATER;
		case COMPARE_NOTEQUAL:
			return GL_NOTEQUAL;
		case COMPARE_GREATEREQUAL:
			return GL_GEQUAL;
		case COMPARE_ALWAYS:
			return GL_ALWAYS;
	}

	return -1;
}

static int shim_stencilop_to_gl(Stencil_Op op)
{
	switch (op) {
		case STENCILOP_KEEP:
			return GL_KEEP;
		case STENCILOP_ZERO:
			return GL_ZERO;
		case STENCILOP_REPLACE:
			return GL_REPLACE;
		case STENCILOP_INCRSAT:
			return GL_INCR;
		case STENCILOP_DECRSAT:
			return GL_DECR;
		case STENCILOP_INVERT:
			return GL_INVERT;
#ifndef ANDROID
		case STENCILOP_INCR:
			return GL_INCR_WRAP;
		case STENCILOP_DECR:
			return GL_DECR_WRAP;
#else
		default:
			return -1;
#endif
	}

	return -1;
}

static int shim_blend_to_gl(Blend_Mode m)
{
	if (m == BLEND_ZERO) {
		return GL_ZERO;
	}
	else if (m == BLEND_ONE) {
		return GL_ONE;
	}
	else if (m == BLEND_SRCCOLOR) {
		return GL_SRC_COLOR;
	}
	else if (m == BLEND_INVSRCCOLOR) {
		return GL_ONE_MINUS_SRC_COLOR;
	}
	else if (m == BLEND_SRCALPHA) {
		return GL_SRC_ALPHA;
	}
	else if (m == BLEND_INVSRCALPHA) {
		return GL_ONE_MINUS_SRC_ALPHA;
	}

	return -1;
}

static void set_default_shader()
{
	shim::current_shader = shim::default_shader;
	shim::current_shader->use();
	update_projection();
}

// window_w/h are passed back out (restart needs them)
static void create_window(int scaled_w, int scaled_h, bool force_integer_scaling, int &window_w, int &window_h)
{
	set_opengl();

#if defined __linux__ && !defined ANDROID
	std::string env = std::string("SDL_VIDEO_FULLSCREEN_HEAD=") + util::itos(shim::adapter);
	putenv((char *)env.c_str());
#endif
	
	util::JSON::Node *root = shim::shim_json->get_root();
	internal::gfx_context.fullscreen = root->get_nested_bool("shim>gfx>fullscreen", &internal::gfx_context.fullscreen, false, true, true);
	vsync = root->get_nested_bool("shim>gfx>vsync", &vsync, true, true, true);
	show_fps = root->get_nested_bool("shim>gfx>show_fps", &show_fps, false);

	screen_shake_end = SDL_GetTicks();
	shim::using_screen_shake = false;

	internal::gfx_context.fullscreen = util::bool_arg(internal::gfx_context.fullscreen, shim::argc, shim::argv, "fullscreen");
	vsync = util::bool_arg(vsync, shim::argc, shim::argv, "vsync");
	show_fps = util::bool_arg(show_fps, shim::argc, shim::argv, "fps");

	int scale_index = util::check_args(shim::argc, shim::argv, "+scale");
	if (scale_index > 0 && scaled_w > 0 && scaled_h > 0) {
		int scale = atoi(shim::argv[scale_index+1]);
		int orig = scale;
		if (scale > 0) {
			bool changed = false;
			while (scale * scaled_w < minimum_window_size.w || scale * scaled_h < minimum_window_size.h) {
				scale++;
				changed = true;
			}
			if (changed) {
				util::infomsg("Changing scale to %d (%d too small!)\n", scale, orig);
			}
			window_w = scaled_w * scale;
			window_h = scaled_h * scale;
		}
	}

	int width_index = util::check_args(shim::argc, shim::argv, "+width");
	int height_index = util::check_args(shim::argc, shim::argv, "+height");
	if (width_index > 0) {
		window_w = atoi(shim::argv[width_index+1]);
	}
	if (height_index > 0) {
		window_h = atoi(shim::argv[height_index+1]);
	}

	::scaled_w = scaled_w;
	::scaled_h = scaled_h;

	::force_integer_scaling = force_integer_scaling;

	internal::gfx_context.fullscreen_window = false;
	
	if (window_w < 0 && window_h < 0 && scaled_w <= 0 && scaled_h <= 0) {
		internal::gfx_context.fullscreen = true; // try and go fullscreen because we have no good information
	}

	SDL_DisplayMode mode;

#ifdef IOS
	SDL_GetDesktopDisplayMode(shim::adapter, &mode);
	window_w = mode.w;
	window_h = mode.h;
#endif

	int flags = 0;

	flags |= internal::gfx_context.fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE;

	if (shim::opengl) {
		flags |= SDL_WINDOW_OPENGL;
	}

	int centre_y;
#if defined IOS || defined ANDROID
	flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI;
	centre_y = 0;
#elif defined RASPBERRYPI
	flags |= SDL_WINDOW_FULLSCREEN;
#else
	util::Size<int> desktop_size = get_desktop_resolution();
#ifdef __linux__
	centre_y = int((desktop_size.h-window_h)/2*0.5f); // slightly up
#else
	centre_y = int((desktop_size.h-window_h)/2*0.75f); // slightly up
#endif
#endif

	if (shim::hide_window) {
		flags |= SDL_WINDOW_HIDDEN;
	}

	if (internal::gfx_context.inited == false && internal::gfx_context.restarting == false && internal::gfx_context.fullscreen) {
		int ret = internal::My_SDL_GetCurrentDisplayMode(shim::adapter, &mode);
		
		if (ret == 0) {
			if ((mode.w > mode.h) != (window_w > window_h)) {
				int tmp = window_w;
				window_w = window_h;
				window_h = tmp;
			}
		}
	}

	int win_x;
	int win_y;

	if (internal::gfx_context.fullscreen) {
		// On Windows these MUST be set like this for OpenGL
		SDL_Rect r;
		SDL_GetDisplayBounds(shim::adapter, &r);
		win_x = r.x;
		win_y = r.y;
	}
	else {
		win_x = SDL_WINDOWPOS_CENTERED_DISPLAY(shim::adapter);
		SDL_Rect r;
		SDL_GetDisplayBounds(shim::adapter, &r);
		win_y = r.y + centre_y;
	}

#if defined _WIN32
	if (shim::opengl && internal::gfx_context.restarting) {
		SDL_Delay(2500); // need this when "restart"ing, need to let old fullscreen mode die fully, especially on screen rotations
	}
#endif
	
	internal::gfx_context.window = SDL_CreateWindow(shim::window_title.c_str(), win_x, win_y, window_w, window_h, flags);
	
	// I guess on Android the window IS 0
#if !defined ANDROID
	if (internal::gfx_context.window == 0) {
		throw util::Error("SDL_CreateWindow failed (" + std::string(SDL_GetError()) + ")");
	}
#endif

#if defined _WIN32
	SDL_SysWMinfo wm_info;
	SDL_VERSION(&wm_info.version);
	SDL_GetWindowWMInfo(internal::gfx_context.window, &wm_info);
	internal::gfx_context.hwnd = wm_info.info.win.window;
	paint_window_black(window_w, window_h);
#endif

#if defined _WIN32
	if (internal::gfx_context.fullscreen) {
		paint_window_black(window_w, window_h);
	}
#elif defined __linux__ && !defined ANDROID
	SDL_SysWMinfo wm_info;
	SDL_VERSION(&wm_info.version);
	SDL_GetWindowWMInfo(internal::gfx_context.window, &wm_info);
	internal::gfx_context.x_display = wm_info.info.x11.display;
	internal::gfx_context.x_window = wm_info.info.x11.window;
#elif defined __APPLE__ && !defined IOS
	SDL_SysWMinfo wm_info;
	SDL_VERSION(&wm_info.version);
	SDL_GetWindowWMInfo(internal::gfx_context.window, &wm_info);
	macosx_centre_window(wm_info.info.cocoa.window);
	macosx_set_background_colour(wm_info.info.cocoa.window, shim::black);
#endif

	internal::gfx_context.windowid = SDL_GetWindowID(internal::gfx_context.window);

	int w = 0;
	int h = 0;
    
	if (shim::opengl) {
		internal::gfx_context.opengl_context = SDL_GL_CreateContext(internal::gfx_context.window);

		if (internal::gfx_context.opengl_context == 0) {
			util::errormsg("Failed to create OpenGL context! (%s)\n", SDL_GetError());
		}

#if !defined ANDROID && !defined IOS
		glStencilFuncSeparate_ptr = (glStencilFuncSeparate_func)SDL_GL_GetProcAddress("glStencilFuncSeparate");
		glStencilOpSeparate_ptr = (glStencilOpSeparate_func)SDL_GL_GetProcAddress("glStencilOpSeparate");
		glBindFramebuffer_ptr = (glBindFramebuffer_func)SDL_GL_GetProcAddress("glBindFramebuffer");
		glDeleteRenderbuffers_ptr = (glDeleteRenderbuffers_func)SDL_GL_GetProcAddress("glDeleteRenderbuffers");
		glGenFramebuffers_ptr = (glGenFramebuffers_func)SDL_GL_GetProcAddress("glGenFramebuffers");
		glGenRenderbuffers_ptr = (glGenRenderbuffers_func)SDL_GL_GetProcAddress("glGenRenderbuffers");
		glBindRenderbuffer_ptr = (glBindRenderbuffer_func)SDL_GL_GetProcAddress("glBindRenderbuffer");
		glFramebufferTexture2D_ptr = (glFramebufferTexture2D_func)SDL_GL_GetProcAddress("glFramebufferTexture2D");
		glRenderbufferStorage_ptr = (glRenderbufferStorage_func)SDL_GL_GetProcAddress("glRenderbufferStorage");
		glCheckFramebufferStatus_ptr = (glCheckFramebufferStatus_func)SDL_GL_GetProcAddress("glCheckFramebufferStatus");
		glDeleteFramebuffers_ptr = (glDeleteFramebuffers_func)SDL_GL_GetProcAddress("glDeleteFramebuffers");
		glFramebufferRenderbuffer_ptr = (glFramebufferRenderbuffer_func)SDL_GL_GetProcAddress("glFramebufferRenderbuffer");
		glUseProgram_ptr = (glUseProgram_func)SDL_GL_GetProcAddress("glUseProgram");
		glUniform1f_ptr = (glUniform1f_func)SDL_GL_GetProcAddress("glUniform1f");
		glUniform2f_ptr = (glUniform2f_func)SDL_GL_GetProcAddress("glUniform2f");
		glUniform3f_ptr = (glUniform3f_func)SDL_GL_GetProcAddress("glUniform3f");
		glUniform4f_ptr = (glUniform4f_func)SDL_GL_GetProcAddress("glUniform4f");
		glUniform1i_ptr = (glUniform1i_func)SDL_GL_GetProcAddress("glUniform1i");
		glUniform2i_ptr = (glUniform2i_func)SDL_GL_GetProcAddress("glUniform2i");
		glUniform3i_ptr = (glUniform3i_func)SDL_GL_GetProcAddress("glUniform3i");
		glUniform4i_ptr = (glUniform4i_func)SDL_GL_GetProcAddress("glUniform4i");
		glUniform1fv_ptr = (glUniform1fv_func)SDL_GL_GetProcAddress("glUniform1fv");
		glUniform2fv_ptr = (glUniform2fv_func)SDL_GL_GetProcAddress("glUniform2fv");
		glUniform3fv_ptr = (glUniform3fv_func)SDL_GL_GetProcAddress("glUniform3fv");
		glUniform4fv_ptr = (glUniform4fv_func)SDL_GL_GetProcAddress("glUniform4fv");
		glUniform1iv_ptr = (glUniform1iv_func)SDL_GL_GetProcAddress("glUniform1iv");
		glUniform2iv_ptr = (glUniform2iv_func)SDL_GL_GetProcAddress("glUniform2iv");
		glUniform3iv_ptr = (glUniform3iv_func)SDL_GL_GetProcAddress("glUniform3iv");
		glUniform4iv_ptr = (glUniform4iv_func)SDL_GL_GetProcAddress("glUniform4iv");
		glUniformMatrix2fv_ptr = (glUniformMatrix2fv_func)SDL_GL_GetProcAddress("glUniformMatrix2fv");
		glUniformMatrix3fv_ptr = (glUniformMatrix3fv_func)SDL_GL_GetProcAddress("glUniformMatrix3fv");
		glUniformMatrix4fv_ptr = (glUniformMatrix4fv_func)SDL_GL_GetProcAddress("glUniformMatrix4fv");
		glDeleteShader_ptr = (glDeleteShader_func)SDL_GL_GetProcAddress("glDeleteShader");
		glCreateProgram_ptr = (glCreateProgram_func)SDL_GL_GetProcAddress("glCreateProgram");
		glDeleteProgram_ptr = (glDeleteProgram_func)SDL_GL_GetProcAddress("glDeleteProgram");
		glAttachShader_ptr = (glAttachShader_func)SDL_GL_GetProcAddress("glAttachShader");
		glLinkProgram_ptr = (glLinkProgram_func)SDL_GL_GetProcAddress("glLinkProgram");
		glGetAttribLocation_ptr = (glGetAttribLocation_func)SDL_GL_GetProcAddress("glGetAttribLocation");
		glGetTexImage_ptr = (glGetTexImage_func)SDL_GL_GetProcAddress("glGetTexImage");
		glEnableVertexAttribArray_ptr = (glEnableVertexAttribArray_func)SDL_GL_GetProcAddress("glEnableVertexAttribArray");
		glVertexAttribPointer_ptr = (glVertexAttribPointer_func)SDL_GL_GetProcAddress("glVertexAttribPointer");
		glGetUniformLocation_ptr = (glGetUniformLocation_func)SDL_GL_GetProcAddress("glGetUniformLocation");
		glShaderSource_ptr = (glShaderSource_func)SDL_GL_GetProcAddress("glShaderSource");
		glCompileShader_ptr = (glCompileShader_func)SDL_GL_GetProcAddress("glCompileShader");
		glGetShaderiv_ptr = (glGetShaderiv_func)SDL_GL_GetProcAddress("glGetShaderiv");
		glGetShaderInfoLog_ptr = (glGetShaderInfoLog_func)SDL_GL_GetProcAddress("glGetShaderInfoLog");
		glCreateShader_ptr = (glCreateShader_func)SDL_GL_GetProcAddress("glCreateShader");
		glBlendFunc_ptr = (glBlendFunc_func)SDL_GL_GetProcAddress("glBlendFunc");
		glEnable_ptr = (glEnable_func)SDL_GL_GetProcAddress("glEnable");
		glDisable_ptr = (glDisable_func)SDL_GL_GetProcAddress("glDisable");
		glFrontFace_ptr = (glFrontFace_func)SDL_GL_GetProcAddress("glFrontFace");
		glCullFace_ptr = (glCullFace_func)SDL_GL_GetProcAddress("glCullFace");
		glScissor_ptr = (glScissor_func)SDL_GL_GetProcAddress("glScissor");
		glViewport_ptr = (glViewport_func)SDL_GL_GetProcAddress("glViewport");
		glClearColor_ptr = (glClearColor_func)SDL_GL_GetProcAddress("glClearColor");
		glClear_ptr = (glClear_func)SDL_GL_GetProcAddress("glClear");
#if defined __APPLE__ && !defined IOS
		glClearDepthf_ptr = (glClearDepthf_func)SDL_GL_GetProcAddress("glClearDepth");
#else
		glClearDepthf_ptr = (glClearDepthf_func)SDL_GL_GetProcAddress("glClearDepthf");
#endif
#if !defined ANDROID && !defined IOS
		if (glClearDepthf_ptr == 0) {
			glClearDepth_ptr = (glClearDepth_func)SDL_GL_GetProcAddress("glClearDepth");
		}
		else {
			glClearDepth_ptr = 0;
		}
#endif
		glClearStencil_ptr = (glClearStencil_func)SDL_GL_GetProcAddress("glClearStencil");
		glDepthMask_ptr = (glDepthMask_func)SDL_GL_GetProcAddress("glDepthMask");
		glDepthFunc_ptr = (glDepthFunc_func)SDL_GL_GetProcAddress("glDepthFunc");
		glStencilFunc_ptr = (glStencilFunc_func)SDL_GL_GetProcAddress("glStencilFunc");
		glStencilOp_ptr = (glStencilOp_func)SDL_GL_GetProcAddress("glStencilOp");
		glStencilFuncSeparate_ptr = (glStencilFuncSeparate_func)SDL_GL_GetProcAddress("glStencilFuncSeparate");
		glStencilOpSeparate_ptr = (glStencilOpSeparate_func)SDL_GL_GetProcAddress("glStencilOpSeparate");
		glBlendFunc_ptr = (glBlendFunc_func)SDL_GL_GetProcAddress("glBlendFunc");
		glActiveTexture_ptr = (glActiveTexture_func)SDL_GL_GetProcAddress("glActiveTexture");
		glColorMask_ptr = (glColorMask_func)SDL_GL_GetProcAddress("glColorMask");
		glDeleteTextures_ptr = (glDeleteTextures_func)SDL_GL_GetProcAddress("glDeleteTextures");
		glGenTextures_ptr = (glGenTextures_func)SDL_GL_GetProcAddress("glGenTextures");
		glBindTexture_ptr = (glBindTexture_func)SDL_GL_GetProcAddress("glBindTexture");
		glTexImage2D_ptr = (glTexImage2D_func)SDL_GL_GetProcAddress("glTexImage2D");
		glTexParameteri_ptr = (glTexParameteri_func)SDL_GL_GetProcAddress("glTexParameteri");
		glGetError_ptr = (glGetError_func)SDL_GL_GetProcAddress("glGetError");
		glDrawArrays_ptr = (glDrawArrays_func)SDL_GL_GetProcAddress("glDrawArrays");

		if (glStencilFuncSeparate_ptr == 0) { util::debugmsg("glStencilFuncSeparate_ptr=%p\n", glStencilFuncSeparate_ptr); }
		if (glStencilOpSeparate_ptr == 0) { util::debugmsg("glStencilOpSeparate_ptr=%p\n", glStencilOpSeparate_ptr); }
		if (glBindFramebuffer_ptr == 0) { util::debugmsg("glBindFramebuffer_ptr=%p\n", glBindFramebuffer_ptr); }
		if (glDeleteRenderbuffers_ptr == 0) { util::debugmsg("glDeleteRenderbuffers_ptr=%p\n", glDeleteRenderbuffers_ptr); }
		if (glGenFramebuffers_ptr == 0) { util::debugmsg("glGenFramebuffers_ptr=%p\n", glGenFramebuffers_ptr); }
		if (glGenRenderbuffers_ptr == 0) { util::debugmsg("glGenRenderbuffers_ptr=%p\n", glGenRenderbuffers_ptr); }
		if (glBindRenderbuffer_ptr == 0) { util::debugmsg("glBindRenderbuffer_ptr=%p\n", glBindRenderbuffer_ptr); }
		if (glFramebufferTexture2D_ptr == 0) { util::debugmsg("glFramebufferTexture2D_ptr=%p\n", glFramebufferTexture2D_ptr); }
		if (glRenderbufferStorage_ptr == 0) { util::debugmsg("glRenderbufferStorage_ptr=%p\n", glRenderbufferStorage_ptr); }
		if (glCheckFramebufferStatus_ptr == 0) { util::debugmsg("glCheckFramebufferStatus_ptr=%p\n", glCheckFramebufferStatus_ptr); }
		if (glDeleteFramebuffers_ptr == 0) { util::debugmsg("glDeleteFramebuffers_ptr=%p\n", glDeleteFramebuffers_ptr); }
		if (glFramebufferRenderbuffer_ptr == 0) { util::debugmsg("glFramebufferRenderbuffer_ptr=%p\n", glFramebufferRenderbuffer_ptr); }
		if (glUseProgram_ptr == 0) { util::debugmsg("glUseProgram_ptr=%p\n", glUseProgram_ptr); }
		if (glUniform1f_ptr == 0) { util::debugmsg("glUniform1f_ptr=%p\n", glUniform1f_ptr); }
		if (glUniform2f_ptr == 0) { util::debugmsg("glUniform2f_ptr=%p\n", glUniform2f_ptr); }
		if (glUniform3f_ptr == 0) { util::debugmsg("glUniform3f_ptr=%p\n", glUniform3f_ptr); }
		if (glUniform4f_ptr == 0) { util::debugmsg("glUniform4f_ptr=%p\n", glUniform4f_ptr); }
		if (glUniform1i_ptr == 0) { util::debugmsg("glUniform1i_ptr=%p\n", glUniform1i_ptr); }
		if (glUniform2i_ptr == 0) { util::debugmsg("glUniform2i_ptr=%p\n", glUniform2i_ptr); }
		if (glUniform3i_ptr == 0) { util::debugmsg("glUniform3i_ptr=%p\n", glUniform3i_ptr); }
		if (glUniform4i_ptr == 0) { util::debugmsg("glUniform4i_ptr=%p\n", glUniform4i_ptr); }
		if (glUniform1fv_ptr == 0) { util::debugmsg("glUniform1fv_ptr=%p\n", glUniform1fv_ptr); }
		if (glUniform2fv_ptr == 0) { util::debugmsg("glUniform2fv_ptr=%p\n", glUniform2fv_ptr); }
		if (glUniform3fv_ptr == 0) { util::debugmsg("glUniform3fv_ptr=%p\n", glUniform3fv_ptr); }
		if (glUniform4fv_ptr == 0) { util::debugmsg("glUniform4fv_ptr=%p\n", glUniform4fv_ptr); }
		if (glUniform1iv_ptr == 0) { util::debugmsg("glUniform1iv_ptr=%p\n", glUniform1iv_ptr); }
		if (glUniform2iv_ptr == 0) { util::debugmsg("glUniform2iv_ptr=%p\n", glUniform2iv_ptr); }
		if (glUniform3iv_ptr == 0) { util::debugmsg("glUniform3iv_ptr=%p\n", glUniform3iv_ptr); }
		if (glUniform4iv_ptr == 0) { util::debugmsg("glUniform4iv_ptr=%p\n", glUniform4iv_ptr); }
		if (glUniformMatrix2fv_ptr == 0) { util::debugmsg("glUniformMatrix2fv_ptr=%p\n", glUniformMatrix2fv_ptr); }
		if (glUniformMatrix3fv_ptr == 0) { util::debugmsg("glUniformMatrix3fv_ptr=%p\n", glUniformMatrix3fv_ptr); }
		if (glUniformMatrix4fv_ptr == 0) { util::debugmsg("glUniformMatrix4fv_ptr=%p\n", glUniformMatrix4fv_ptr); }
		if (glDeleteShader_ptr == 0) { util::debugmsg("glDeleteShader_ptr=%p\n", glDeleteShader_ptr); }
		if (glCreateProgram_ptr == 0) { util::debugmsg("glCreateProgram_ptr=%p\n", glCreateProgram_ptr); }
		if (glDeleteProgram_ptr == 0) { util::debugmsg("glDeleteProgram_ptr=%p\n", glDeleteProgram_ptr); }
		if (glAttachShader_ptr == 0) { util::debugmsg("glAttachShader_ptr=%p\n", glAttachShader_ptr); }
		if (glLinkProgram_ptr == 0) { util::debugmsg("glLinkProgram_ptr=%p\n", glLinkProgram_ptr); }
		if (glGetAttribLocation_ptr == 0) { util::debugmsg("glGetAttribLocation_ptr=%p\n", glGetAttribLocation_ptr); }
		if (glGetTexImage_ptr == 0) { util::debugmsg("glGetTexImage_ptr=%p\n", glGetTexImage_ptr); }
		if (glEnableVertexAttribArray_ptr == 0) { util::debugmsg("glEnableVertexAttribArray_ptr=%p\n", glEnableVertexAttribArray_ptr); }
		if (glVertexAttribPointer_ptr == 0) { util::debugmsg("glVertexAttribPointer_ptr=%p\n", glVertexAttribPointer_ptr); }
		if (glGetUniformLocation_ptr == 0) { util::debugmsg("glGetUniformLocation_ptr=%p\n", glGetUniformLocation_ptr); }
		if (glShaderSource_ptr == 0) { util::debugmsg("glShaderSource_ptr=%p\n", glShaderSource_ptr); }
		if (glCompileShader_ptr == 0) { util::debugmsg("glCompileShader_ptr=%p\n", glCompileShader_ptr); }
		if (glGetShaderiv_ptr == 0) { util::debugmsg("glGetShaderiv_ptr=%p\n", glGetShaderiv_ptr); }
		if (glGetShaderInfoLog_ptr == 0) { util::debugmsg("glGetShaderInfoLog_ptr=%p\n", glGetShaderInfoLog_ptr); }
		if (glCreateShader_ptr == 0) { util::debugmsg("glCreateShader_ptr=%p\n", glCreateShader_ptr); }
		if (glBlendFunc_ptr == 0) { util::debugmsg("glBlendFunc_ptr=%p\n", glBlendFunc_ptr); }
		if (glEnable_ptr == 0) { util::debugmsg("glEnable_ptr=%p\n", glEnable_ptr); }
		if (glDisable_ptr == 0) { util::debugmsg("glDisable_ptr=%p\n", glDisable_ptr); }
		if (glFrontFace_ptr == 0) { util::debugmsg("glFrontFace_ptr=%p\n", glFrontFace_ptr); }
		if (glCullFace_ptr == 0) { util::debugmsg("glCullFace_ptr=%p\n", glCullFace_ptr); }
		if (glScissor_ptr == 0) { util::debugmsg("glScissor_ptr=%p\n", glScissor_ptr); }
		if (glViewport_ptr == 0) { util::debugmsg("glViewport_ptr=%p\n", glViewport_ptr); }
		if (glClearColor_ptr == 0) { util::debugmsg("glClearColor_ptr=%p\n", glClearColor_ptr); }
		if (glClear_ptr == 0) { util::debugmsg("glClear_ptr=%p\n", glClear_ptr); }
		if (glClearDepthf_ptr == 0) { util::debugmsg("glClearDepthf_ptr=%p\n", glClearDepthf_ptr); }
		if (glClearStencil_ptr == 0) { util::debugmsg("glClearStencil_ptr=%p\n", glClearStencil_ptr); }
		if (glDepthMask_ptr == 0) { util::debugmsg("glDepthMask_ptr=%p\n", glDepthMask_ptr); }
		if (glDepthFunc_ptr == 0) { util::debugmsg("glDepthFunc_ptr=%p\n", glDepthFunc_ptr); }
		if (glStencilFunc_ptr == 0) { util::debugmsg("glStencilFunc_ptr=%p\n", glStencilFunc_ptr); }
		if (glStencilOp_ptr == 0) { util::debugmsg("glStencilOp_ptr=%p\n", glStencilOp_ptr); }
		if (glStencilFuncSeparate_ptr == 0) { util::debugmsg("glStencilFuncSeparate_ptr=%p\n", glStencilFuncSeparate_ptr); }
		if (glStencilOpSeparate_ptr == 0) { util::debugmsg("glStencilOpSeparate_ptr=%p\n", glStencilOpSeparate_ptr); }
		if (glBlendFunc_ptr == 0) { util::debugmsg("glBlendFunc_ptr=%p\n", glBlendFunc_ptr); }
		if (glActiveTexture_ptr == 0) { util::debugmsg("glActiveTexture_ptr=%p\n", glActiveTexture_ptr); }
		if (glColorMask_ptr == 0) { util::debugmsg("glColorMask_ptr=%p\n", glColorMask_ptr); }
		if (glDeleteTextures_ptr == 0) { util::debugmsg("glDeleteTextures_ptr=%p\n", glDeleteTextures_ptr); }
		if (glGenTextures_ptr == 0) { util::debugmsg("glGenTextures_ptr=%p\n", glGenTextures_ptr); }
		if (glBindTexture_ptr == 0) { util::debugmsg("glBindTexture_ptr=%p\n", glBindTexture_ptr); }
		if (glTexImage2D_ptr == 0) { util::debugmsg("glTexImage2D_ptr=%p\n", glTexImage2D_ptr); }
		if (glTexParameteri_ptr == 0) { util::debugmsg("glTexParameteri_ptr=%p\n", glTexParameteri_ptr); }
		if (glGetError_ptr == 0) { util::debugmsg("glGetError_ptr=%p\n", glGetError_ptr); }
		if (glDrawArrays_ptr == 0) { util::debugmsg("glDrawArrays_ptr=%p\n", glDrawArrays_ptr); }
#endif

		gfx::clear(shim::black);
		flip();

#ifdef IOS
		bool v = 1;
#else
		bool v = vsync;
#endif

		SDL_GL_SetSwapInterval(v ? 1 : 0); // vsync, 1 = on

#if defined IOS
	       SDL_GL_GetDrawableSize(internal::gfx_context.window, &w, &h);
#else
	       if (internal::gfx_context.fullscreen) {
		       w = window_w;
		       h = window_h;
	       }
	       else {
		       SDL_GetWindowSize(internal::gfx_context.window, &w, &h);
	       }
#endif

#if defined IOS
		SDL_SysWMinfo wm_info;
		SDL_VERSION(&wm_info.version);
		SDL_GetWindowWMInfo(internal::gfx_context.window, &wm_info);
		internal::gfx_context.framebuffer = wm_info.info.uikit.framebuffer;
		internal::gfx_context.colorbuffer = wm_info.info.uikit.colorbuffer;
#endif

		glEnable_ptr(GL_BLEND);
		PRINT_GL_ERROR("glEnable\n");
		glBlendFunc_ptr(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		PRINT_GL_ERROR("glBlendFunc\n");
		glEnable_ptr(GL_CULL_FACE);
		PRINT_GL_ERROR("glEnable_ptr(GL_CULL_FACE)\n");
		glFrontFace_ptr(GL_CW);
		PRINT_GL_ERROR("glFrontFace\n");
		glCullFace_ptr(GL_BACK);
		PRINT_GL_ERROR("glFrontFace\n");
	}
#ifdef _WIN32
	else {
		if (internal::gfx_context.fullscreen) {
			w = window_w;
			h = window_h;
		}
		else {
			SDL_GetWindowSize(internal::gfx_context.window, &w, &h);
		}

		fill_d3d_pp(w, h);

		create_d3d_device();
		
		clear(shim::black);
		flip();
	}
#endif

	shim::real_screen_size = {w, h};
}

static void destroy_window(bool destroy_d3d = false)
{
#ifdef _WIN32
	if (icon_small != 0) {
		SetClassLongPtr(internal::gfx_context.hwnd, GCLP_HICONSM, (LONG_PTR)nullptr);
		DestroyIcon(icon_small);
	}
	if (icon_big != 0) {
		SetClassLongPtr(internal::gfx_context.hwnd, GCLP_HICON, (LONG_PTR)nullptr);
		DestroyIcon(icon_big);
	}
#endif

	if (shim::opengl) {
		SDL_GL_DeleteContext(internal::gfx_context.opengl_context);
		SDL_DestroyWindow(internal::gfx_context.window);
	}
#ifdef _WIN32
	else {
		util::verbosemsg("d3d_device->Release=%d\n", shim::d3d_device->Release());
		d3d_device_count--;
		SDL_DestroyWindow(internal::gfx_context.window);
		if (destroy_d3d) {
			if (shim::opengl == false) {
				util::verbosemsg("d3d->Release=%d\n", d3d->Release());
				d3d_count--;
			}
		}
	}
#endif
}

static void start_video(int scaled_w, int scaled_h, bool force_integer_scaling, int window_w, int window_h)
{
	int index;
	if ((index = util::check_args(shim::argc, shim::argv, "+adapter")) > 0) {
		shim::adapter = atoi(shim::argv[index+1]);
		if (shim::adapter > SDL_GetNumVideoDisplays()-1) {
			shim::adapter = 0;
		}
	}

#ifdef _WIN32
	if (util::bool_arg(false, shim::argc, shim::argv, "opengl") == false) {
		if ((d3d = Direct3DCreate9(D3D_SDK_VERSION)) == 0) {
			throw util::Error("Direct3D9Create failed");
		}
		else {
			d3d_count++;
		}
	}
#endif

	create_window(scaled_w, scaled_h, force_integer_scaling, window_w, window_h);

#ifdef _WIN32
	if (shim::opengl == false) {
		d3d_create_depth_buffer();

		set_initial_d3d_state();

		if (internal::gfx_context.render_target == 0) {
			if (shim::d3d_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &internal::gfx_context.render_target) != D3D_OK) {
				util::infomsg("GetBackBuffer failed after CreateDevice.\n");
			}
		}
	}
#endif

	if (shim::opengl) {
		std::string default_vertex_source = DEFAULT_GLSL_VERTEX_SHADER;
		std::string default_fragment_source = DEFAULT_GLSL_FRAGMENT_SHADER;
		std::string default_textured_fragment_source = DEFAULT_GLSL_TEXTURED_FRAGMENT_SHADER;
		std::string model_vertex_source = MODEL_GLSL_VERTEX_SHADER;
		std::string model_fragment_source = MODEL_GLSL_FRAGMENT_SHADER;
		std::string appear_fragment_source = APPEAR_GLSL_FRAGMENT_SHADER;
		Shader::OpenGL_Shader *default_vertex = Shader::load_opengl_vertex_shader(DEFAULT_GLSL_VERTEX_SHADER, Shader::HIGH);
		Shader::OpenGL_Shader *default_fragment = Shader::load_opengl_fragment_shader(DEFAULT_GLSL_FRAGMENT_SHADER);
		Shader::OpenGL_Shader *default_textured_fragment = Shader::load_opengl_fragment_shader(DEFAULT_GLSL_TEXTURED_FRAGMENT_SHADER, Shader::HIGH);
		Shader::OpenGL_Shader *model_vertex = Shader::load_opengl_vertex_shader(MODEL_GLSL_VERTEX_SHADER);
		Shader::OpenGL_Shader *model_fragment = Shader::load_opengl_fragment_shader(MODEL_GLSL_FRAGMENT_SHADER);
		Shader::OpenGL_Shader *appear_fragment = Shader::load_opengl_fragment_shader(APPEAR_GLSL_FRAGMENT_SHADER);
		shim::default_shader = internal::gfx_context.untextured_shader = new Shader(default_vertex, default_fragment, true, true);
		internal::gfx_context.textured_shader = new Shader(default_vertex, default_textured_fragment, false, true);
		shim::model_shader = new Shader(model_vertex, model_fragment, true, true);
		shim::appear_shader = new Shader(default_vertex, appear_fragment, false, true);
	}
#ifdef _WIN32
	else {
#ifdef USE_D3DX
		default_vertex_source = DEFAULT_HLSL_VERTEX_SHADER;
		default_fragment_source = DEFAULT_HLSL_FRAGMENT_SHADER;
		default_textured_fragment_source = DEFAULT_HLSL_TEXTURED_FRAGMENT_SHADER;
		model_fragment_source = MODEL_HLSL_FRAGMENT_SHADER;
	
		shim::default_shader = internal::gfx_context.untextured_shader = new Shader(shim::opengl, default_vertex_source, default_fragment_source);
		internal::gfx_context.textured_shader = new Shader(shim::opengl, default_vertex_source, default_textured_fragment_source);
		shim::model_shader = new Shader(shim::opengl, default_vertex_source, model_fragment_source);
#else
		Shader::D3D_Vertex_Shader *default_vertex_shader = Shader::load_d3d_vertex_shader("noo_default_vertex");
		Shader::D3D_Fragment_Shader *default_fragment_shader = Shader::load_d3d_fragment_shader("noo_default_fragment");
		Shader::D3D_Fragment_Shader *default_textured_fragment_shader = Shader::load_d3d_fragment_shader("noo_default_textured_fragment");
		Shader::D3D_Fragment_Shader *model_fragment_shader = Shader::load_d3d_fragment_shader("noo_model_fragment");
		Shader::D3D_Fragment_Shader *appear_fragment_shader = Shader::load_d3d_fragment_shader("appear_fragment");
		shim::default_shader = internal::gfx_context.untextured_shader = new Shader(default_vertex_shader, default_fragment_shader, true, true);
		internal::gfx_context.textured_shader = new Shader(default_vertex_shader, default_textured_fragment_shader, false, true);
		shim::model_shader = new Shader(default_vertex_shader, model_fragment_shader, false, true);
		shim::appear_shader = new Shader(default_vertex_shader, appear_fragment_shader, false, true);
#endif
	}
#endif

	set_default_shader();

	set_screen_size(shim::real_screen_size);

	util::infomsg("Window size: %dx%d, scaled size: %dx%d.\n", shim::real_screen_size.w, shim::real_screen_size.h, shim::screen_size.w, shim::screen_size.h);
	internal::gfx_context.plasma = gen_plasma(util::rand(0, 10000), 1.0f, 0.0f, shim::white);

	internal::recreate_work_image();

	SDL_DisplayMode mode;
	if (internal::My_SDL_GetCurrentDisplayMode(shim::adapter, &mode) == 0) {
		shim::refresh_rate = mode.refresh_rate;
	}
}

static void end_video()
{
	delete internal::gfx_context.work_image;
	internal::gfx_context.work_image = 0;

	Vertex_Cache::destroy();

	delete shim::default_shader;
	delete internal::gfx_context.textured_shader;
	delete shim::model_shader;
	shim::default_shader = 0;
	internal::gfx_context.textured_shader = 0;
	shim::model_shader = 0;

	destroy_window(true);
}

#if (defined __linux__ && !defined ANDROID) || defined _WIN32
static void set_window_icon()
{
	util::Size<int> size;
	unsigned char *pixels;
	try {
#ifdef _WIN32
		int h = 16;
		std::string filename = std::string("gfx/images/misc/icon") + util::itos(h) + ".tga";
		pixels = Image::read_tga(filename, size);
#else
		pixels = Image::read_tga("gfx/images/misc/icon256.tga", size);
#endif
	}
	catch (util::Error &e) {
		util::infomsg(e.error_message + "\n");
		return;
	}

#if (defined __linux__ && !defined ANDROID)
	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(pixels + size.w * (size.h-1) * 4, size.w, size.h, 32, -size.w * 4, 0xff, 0xff00, 0xff0000, 0xff000000);
	SDL_SetWindowIcon(internal::gfx_context.window, surface);
	SDL_FreeSurface(surface);
#else
	icon_small = internal::win_create_icon(internal::gfx_context.hwnd, (Uint8 *)pixels, size, 0, 0, false);
	SetClassLongPtr(internal::gfx_context.hwnd, GCLP_HICONSM, (LONG_PTR)icon_small);
	delete[] pixels;

	try {
		// Get taskbar height to find a suitable icon size... we pick the nearest value less than the
		// taskbar height minus 10 which works well.
		RECT r;
		if (SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0)) {
			int workarea_h = r.bottom - r.top;
			int full_h = GetSystemMetrics(SM_CYSCREEN);
			int taskbar_h = full_h - workarea_h;
			int available[] = { 16, /*20,*/ 24, 32, 48, 0 };
			int biggest = 0;
			int o;
			OSVERSIONINFO vi;
			memset(&vi, 0, sizeof(vi));
			vi.dwOSVersionInfoSize = sizeof(vi);
			// Windows 7 and 8/8.1 have 32 pixel icons on a 40 pixel taskbar
#pragma warning(push)
#pragma warning(disable : 4996)
			if (GetVersionEx(&vi) && vi.dwMajorVersion == 6 && std::abs(40-taskbar_h) <= 5u) {
				o = 0;
			}
#pragma warning(pop)
			else {
				o = -10; // there is some padding
			}
			for (int i = 0; available[i] != 0; i++) {
				if (available[i] < taskbar_h+o && available[i] > biggest) { // not scientific, but works
					biggest = available[i];
				}
			}
			//util::debugmsg("workarea_h=%d taskbar_h=%d o=%d biggest=%d dwMajorVersion=%d\n", workarea_h, taskbar_h, o, biggest, vi.dwMajorVersion);
			std::string filename = std::string("gfx/images/misc/icon") + util::itos(biggest) + ".tga";
			pixels = Image::read_tga(filename, size);
		}
		else {
			throw util::Error("SystemParametersInfo failed!");
		}
	}
	catch (util::Error &e) {
		util::infomsg(e.error_message + "\n");
		try {
			int h = 24;
			std::string filename = std::string("gfx/images/misc/icon") + util::itos(h) + ".tga";
			pixels = Image::read_tga(filename, size);
		}
		catch (util::Error &e) {
			util::infomsg(e.error_message + "\n");
			return;
		}
	}

	icon_big = internal::win_create_icon(internal::gfx_context.hwnd, (Uint8 *)pixels, size, 0, 0, false);
	SetClassLongPtr(internal::gfx_context.hwnd, GCLP_HICON, (LONG_PTR)icon_big);
#endif

	delete[] pixels;
}
#endif

#if ((defined __APPLE__ && !defined IOS) || (defined __linux__ && !defined ANDROID && !defined RASPBERRYPI) || defined _WIN32)
void create_mouse_cursors()
{
	// Note: this needs to be a specific size on Windows, 32x32 works for me
	util::Size<int> size;
	unsigned char *pixels;

	if (shim::scale_mouse_cursor) {
		gfx::Image *mc;
		try {
			mc = new gfx::Image("ui/mouse_cursor.tga");
		}
		catch (util::Error &e) {
			util::infomsg(e.error_message + "\n");
			use_custom_cursor = false;
			return;
		}
		gfx::Image *scaled = new gfx::Image(mc->size*shim::scale);
		gfx::Image *target = gfx::get_target_image();
		gfx::set_target_image(scaled);
		gfx::clear(shim::transparent);
		mc->stretch_region({0.0f, 0.0f}, mc->size, {0.0f, 0.0f}, mc->size*(int)shim::scale, 0);
		gfx::set_target_image(target);
		pixels = Image::read_texture(scaled);
		size = scaled->size;
		delete mc;
		delete scaled;
	}
	else {
		try {
			pixels = Image::read_tga("gfx/images/ui/mouse_cursor.tga", size);
		}
		catch (util::Error &e) {
			util::infomsg(e.error_message + "\n");
			use_custom_cursor = false;
			return;
		}
	}

	if (use_custom_cursor) {
		mouse_cursor_surface = SDL_CreateRGBSurfaceFrom(pixels + size.w * (size.h-1) * 4, size.w, size.h, 32, -size.w*4, 0xff, 0xff00, 0xff0000, 0xff000000);
		mouse_cursor = SDL_CreateColorCursor(mouse_cursor_surface, shim::cursor_hotspot.x, shim::cursor_hotspot.y);
	}
	else {
		mouse_cursor = 0;
	}

	delete[] pixels;
}

void delete_mouse_cursors()
{
	if (use_custom_cursor) {
		if (mouse_cursor) {
			SDL_FreeCursor(mouse_cursor);
			mouse_cursor = nullptr;
		}
		if (mouse_cursor_surface) {
			SDL_FreeSurface(mouse_cursor_surface);
			mouse_cursor_surface = nullptr;
		}
	}
}
#endif

static void load_fonts()
{
	int sheet_size = 512;

	while (sheet_size < shim::screen_size.w && sheet_size < shim::screen_size.h) {
		sheet_size *= 2;
	}

	if (sheet_size > 512 && (sheet_size > shim::screen_size.w || sheet_size > shim::screen_size.h)) {
		sheet_size /= 2;
	}

	try {
		shim::font = new TTF("DejaVuSans.ttf", shim::font_size, sheet_size);
	}
	catch (util::Error &e) {
		util::infomsg(e.error_message + "\n");
	}
}

static void destroy_fonts()
{
	delete shim::font;
	shim::font = 0;
}

void real_set_scissor(int x, int y, int w, int h)
{
	if (shim::opengl) {
		int szy;
		if (internal::gfx_context.target_image == 0) {
			szy = shim::real_screen_size.h;
		}
		else {
			szy = internal::gfx_context.target_image->size.h;
		}
		glEnable_ptr(GL_SCISSOR_TEST);
		PRINT_GL_ERROR("glEnable_ptr(GL_SCISSOR_TEST) (%d, %d, %d, %d)\n", x, y, w, h);
		glScissor_ptr(x, szy-h-y, w, h);
		PRINT_GL_ERROR("glScissor");
	}
#ifdef _WIN32
	else {
		shim::d3d_device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
		RECT scissor = { x, y, x+w, y+h };
		shim::d3d_device->SetScissorRect(&scissor);
	}
#endif
}

void get_scissor(int **x, int **y, int **w, int **h)
{
	*x = &scissor_x;
	*y = &scissor_y;
	*w = &scissor_w;
	*h = &scissor_h;
}

void real_set_minimum_window_size(util::Size<int> size)
{
#ifndef IOS
	SDL_SetWindowMinimumSize(internal::gfx_context.window, size.w, size.h);
#endif
}

void real_set_maximum_window_size(util::Size<int> size)
{
#ifndef IOS
	SDL_SetWindowMaximumSize(internal::gfx_context.window, size.w, size.h);
#endif
}

bool static_start()
{
	internal::gfx_context.inited = false;
	internal::gfx_context.restarting = false;
	internal::gfx_context.mouse_in_window = true;

	internal::gfx_context.draw_mutex = SDL_CreateMutex();

	lost_device_callback = 0;
	found_device_callback = 0;
	
	min_aspect_ratio = 4.0f / 3.0f;
	max_aspect_ratio = 16.0f / 9.0f;

	mouse_cursor_shown = true;

	blending_enabled = true;

	minimum_window_size = {-1, -1};
	maximum_window_size = {-1, -1};

	press_and_hold_state = -1;

	last_gui_size = {-1, -1};

	internal::gfx_context.work_image = 0;

	d3d_device_count = 0;
	d3d_count = 0;
	d3d_device_depth_count = 0;

	last_screen_mode = {-1, -1};
	
	handled_lost = false;

#ifdef _WIN32
	internal::gfx_context.depth_stencil_buffer = 0;
	internal::gfx_context.render_target = 0;
#endif

	Shader::static_start();
	Vertex_Cache::static_start();
	Image::static_start();
	Sprite::static_start();
	Font::static_start();
	Tilemap::static_start();
	Model::static_start();
	static_start_primitives();

	return true;
}

void static_end()
{
	SDL_DestroyMutex(internal::gfx_context.draw_mutex);
}

bool start(int scaled_w, int scaled_h, bool force_integer_scaling, int window_w, int window_h)
{
	::create_depth_buffer = shim::create_depth_buffer;
	::create_stencil_buffer = shim::create_stencil_buffer;

	set_opengl();

	if (shim::opengl) {
		SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 0);
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
#if defined IOS || defined ANDROID || defined RASPBERRYPI
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
		if (::create_depth_buffer) {
#if defined ANDROID || defined RASPBERRYPI || defined RASPBERRYPI_NORMAL
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
#else
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#endif
		}
		else {
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
		}
		if (::create_stencil_buffer) {
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		}
		else {
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
		}
	}
	
	::scaled_w = -1;
	::scaled_h = -1;

	total_frames = 0;
	fps = 0;

	notifications.clear();
	fancy_draw_start = 0;
	notification_start_time = 0;

	two_sided_stencil = false;
	depth_write_enabled = false;

	try {
		start_video(scaled_w, scaled_h, force_integer_scaling, window_w, window_h);
	}
	catch (util::Error &e) {
		util::infomsg("start_video failed: " + e.error_message + "\n");
		util::infomsg("Retrying with default w/h...\n");
		start_video(scaled_w, scaled_h, force_integer_scaling, -1, -1);
	}

	/*
	// FIXME:
	D3DADAPTER_IDENTIFIER9 ident;
	if (d3d->GetAdapterIdentifier(shim::adapter, 0, &ident) != D3D_OK) {
		util::debugmsg("GetAdapterIdentifier failed\n");
	}
	else {
		util::debugmsg("gfx card=%s %s\n", ident.Driver, ident.Description);
	}
	*/

	// palette must be loaded before ALL images (cursor etc)
	try {
		shim::palette_size = load_default_palette();
	}
	catch (util::Error &e) {
		util::infomsg(e.error_message + "\n");
	}

#if ((defined __APPLE__ && !defined IOS) || (defined __linux__ && !defined ANDROID && !defined RASPBERRYPI) || defined _WIN32)
	use_custom_cursor = util::bool_arg(true, shim::argc, shim::argv, "custom-cursor");
	if (use_custom_cursor) {
		create_mouse_cursors();
	}
#endif

#if (defined __linux__ && !defined ANDROID) || defined _WIN32
	set_window_icon();

	SDL_PumpEvents(); // without this the icon doesn't appear until the event loop starts
#endif

#if defined _WIN32
	internal::gfx_context.d3d_lost = false;
#endif
	internal::gfx_context.target_image = 0;

#ifdef USE_TTF
	if (TTF_Init() == -1) {
		return false;
	}
#endif

	load_fonts();

	internal::gfx_context.inited = true;

	if (minimum_window_size.w > 0) {
		real_set_minimum_window_size(minimum_window_size);
	}
	if (maximum_window_size.w > 0) {
		real_set_maximum_window_size(maximum_window_size);
	}
	
	return true;
}

// This function "pauses" events (on Linux SDL can spam keys when resizing/recreating windows)
bool restart(int scaled_w, int scaled_h, bool force_integer_scaling, int window_w, int window_h)
{
	SDL_LockMutex(gfx::internal::gfx_context.draw_mutex);
	internal::gfx_context.restarting = true;
	internal::gfx_context.inited = false;
	SDL_UnlockMutex(gfx::internal::gfx_context.draw_mutex);

#if defined __linux__ || defined __APPLE__
	bool is_orientation = false;
#else
	bool is_orientation = internal::gfx_context.fullscreen && ((window_w > window_h) != (shim::real_screen_size.w > shim::real_screen_size.h)); // FIXME: maybe this could be only for D3D (needs lots of testing e.g., tablet, multiple computers)
#endif

	internal::handle_lost_device(true, true);

	if (is_orientation) {
		end_video();
	}
	else {
		destroy_window(false);
	}
	
	internal::gfx_context.fullscreen = false;

	if (is_orientation) {
		start_video(scaled_w, scaled_h, force_integer_scaling, window_w, window_h);
	}
	else {
		create_window(scaled_w, scaled_h, force_integer_scaling, window_w, window_h);
	}

	if (minimum_window_size.w > 0) {
		real_set_minimum_window_size(minimum_window_size);
	}
	if (maximum_window_size.w > 0) {
		real_set_maximum_window_size(maximum_window_size);
	}
	if (press_and_hold_state >= 0) {
		enable_press_and_hold(press_and_hold_state == 1);
	}

#ifdef __linux__ // FIXME: not sure why this is here, if find out, leave a comment
	glViewport_ptr(0, 0, window_w, window_h); // reset in handle_found_device
	PRINT_GL_ERROR("glViewport\n");
	clear(shim::black);
	flip();
	SDL_Delay(100);
#endif
	
	internal::handle_found_device(true, true);
	
#if (defined __linux__ && !defined ANDROID) || defined _WIN32
	set_window_icon();

	SDL_PumpEvents(); // without this the icon doesn't appear until the event loop starts
#endif

	last_screen_mode = {-1, -1};

	SDL_LockMutex(gfx::internal::gfx_context.draw_mutex);
	internal::gfx_context.restarting = false;
	internal::gfx_context.inited = true;
	SDL_UnlockMutex(gfx::internal::gfx_context.draw_mutex);

	return true;
}

void end()
{
	SDL_LockMutex(internal::gfx_context.draw_mutex);
	
	internal::gfx_context.inited = false;
	internal::gfx_context.fullscreen = false;

	Tilemap::release_sheets();

#if ((defined __APPLE__ && !defined IOS) || (defined __linux__ && !defined ANDROID && !defined RASPBERRYPI) || defined _WIN32)
	delete_mouse_cursors();
#endif

	destroy_fonts();
#ifdef USE_TTF
	TTF_Quit();
#endif

	end_video();

	SDL_UnlockMutex(internal::gfx_context.draw_mutex);

	handled_lost = false;
}

void update_animations()
{
	Tilemap::update_all();
	Sprite::update_all();
	Model::update_all();
}

void register_lost_device_callbacks(gfx::_lost_device_callback lost, gfx::_lost_device_callback found)
{
	lost_device_callback = lost;
	found_device_callback = found;
}

void get_lost_device_callbacks(gfx::_lost_device_callback_pointer lost, gfx::_lost_device_callback_pointer found)
{
	*lost = lost_device_callback;
	*found = found_device_callback;
}

Image *get_target_image()
{
	return internal::gfx_context.target_image;
}

void set_target_image(Image *image)
{
	if (image == 0) {
		set_target_backbuffer();
		return;
	}

	SDL_LockMutex(internal::gfx_context.draw_mutex);

	if (internal::gfx_context.target_image != 0) {
		Image *img = internal::gfx_context.target_image;
		internal::gfx_context.target_image = 0;
		img->release_target();
	}

	internal::gfx_context.target_image = image;

	internal::gfx_context.target_image->set_target();

	SDL_UnlockMutex(internal::gfx_context.draw_mutex);
}

void set_target_backbuffer()
{
	if (internal::gfx_context.target_image != 0) {
		SDL_LockMutex(internal::gfx_context.draw_mutex);
		Image *img = internal::gfx_context.target_image;
		internal::gfx_context.target_image = 0;
		img->release_target();
		SDL_UnlockMutex(internal::gfx_context.draw_mutex);
	}
}

void get_matrices(glm::mat4 &mv, glm::mat4 &p)
{
	mv = modelview;
	p = proj;
}

void set_matrices(glm::mat4 &mv, glm::mat4 &p)
{
	modelview = mv;
	proj = p;
}

void set_default_projection(util::Size<int> screen_size, util::Point<int> screen_offset, float scale)
{
	modelview = glm::mat4();
	modelview = glm::translate(modelview, glm::vec3(screen_offset.x, screen_offset.y, 0));
	modelview = glm::scale(modelview, glm::vec3(scale, scale, scale));
	proj = glm::ortho(0.0f, (float)screen_size.w, (float)screen_size.h, 0.0f);

	if (screen_size == shim::real_screen_size && screen_offset == shim::screen_offset && scale == shim::scale) {
		default_modelview = modelview;
		default_proj = proj;
	}
}

void update_projection()
{
	shim::current_shader->set_matrix("modelview", modelview);

	util::Size<int> target_size;
	Image *target = get_target_image();
	if (target == 0) {
		target_size = shim::real_screen_size;
	}
	else {
		target_size = target->size;
	}

	// d3d and opengl pixel coordinates differ
	glm::mat4 d3d_fix = glm::mat4();
	d3d_fix = glm::scale(d3d_fix, glm::vec3(1.0f, 1.0f, 0.5f));
	d3d_fix = glm::translate(d3d_fix, glm::vec3(0.0f, 0.0f, 0.5f));
	d3d_fix = glm::translate(d3d_fix, glm::vec3(-0.5f / (float)target_size.w, 0.5f / (float)target_size.h, 0.0f));
	glm::mat4 p = shim::opengl ? proj : d3d_fix * proj;
	shim::current_shader->set_matrix("proj", p);
}

void set_screen_size(util::Size<int> size)
{
	util::Size<int> orig_size = size;

	if (size.w == scaled_w && size.h == scaled_h) {
		shim::screen_size = size;
		shim::scale = 1.0f;
	}
	else {
		float inv = 1.0f - (shim::black_bar_percent * 2.0f);

		size = {int(inv * size.w), int(inv * size.h)};

		bool int_ok;
		if (force_integer_scaling) {
			int scale = size.w / scaled_w;
			scale = MIN(scale, size.h / scaled_h);
			int rw = size.w / scale;
			int rh = size.h / scale;
			if (rw*scale == size.w && rh*scale == size.h) {
				shim::scale = float(scale);
				shim::screen_size.w = rw;
				shim::screen_size.h = rh;
				int_ok = true;
			}
			else {
				scale--;
				int rw = size.w / scale;
				int rh = size.h / scale;
				if (rw*scale == size.w && rh*scale == size.h) {
					shim::scale = float(scale);
					shim::screen_size.w = rw;
					shim::screen_size.h = rh;
					int_ok = true;
				}
				else {
					int_ok = false;
				}
			}
		}
		else {
			int_ok = false;
		}

		// If the size is much different than the desired size, can't use it
		if (int_ok) {
			float rx = (float)shim::screen_size.w / scaled_w;
			float ry = (float)shim::screen_size.h / scaled_h;
			if (rx < 0.75f || rx > 1.25f || ry < 0.75f || ry > 1.25f) {
				int_ok = false;
			}
		}

		if (int_ok == false) {
			float scale_x = (float)(size.w+1) / scaled_w;
			float scale_y = (float)(size.h+1) / scaled_h;
			float smallest = MIN(scale_x, scale_y);
			shim::scale = smallest;
		}

		float aspect = (float)(size.w / shim::scale) / (size.h / shim::scale);

		if (aspect > max_aspect_ratio || aspect < min_aspect_ratio) {
			if (aspect > max_aspect_ratio) {
				shim::screen_size.h = int(size.h / shim::scale);
				shim::screen_size.w = int((size.h / shim::scale) * max_aspect_ratio);
			}
			else {
				shim::screen_size.w = int(size.w / shim::scale);
				shim::screen_size.h = int((size.w / shim::scale) / min_aspect_ratio);
			}
		}
		else if (int_ok == false) {
			shim::screen_size.w = int(size.w / shim::scale);
			shim::screen_size.h = int(size.h / shim::scale);
		}

		// Make sure entire window is covered
		if (size.w - (shim::screen_size.w * shim::scale) >= 1.0f) {
			shim::screen_size.w++;
		}
		if (size.h - (shim::screen_size.h * shim::scale) >= 1.0f) {
			shim::screen_size.h++;
		}
	}

	if (::scaled_w <= 0 && ::scaled_h <= 0) {
		shim::screen_size = shim::real_screen_size;
		shim::scale = 1.0f;
	}

	if (shim::allow_dpad_below && shim::screen_size.h*shim::scale*1.5f < orig_size.h) {
		shim::screen_offset = {int(orig_size.w-(shim::screen_size.w*shim::scale))/2, int(orig_size.h-(shim::screen_size.h*1.5f*shim::scale))/2}; // room for controls
		shim::dpad_below = true;
	}
	else {
		shim::screen_offset = {int(orig_size.w-(shim::screen_size.w*shim::scale))/2, int(orig_size.h-(shim::screen_size.h*shim::scale))/2};
		shim::dpad_below = false;
	}


	if (shim::screen_offset.x < shim::scale) {
		shim::screen_offset.x = 0;
	}
	if (shim::screen_offset.y < shim::scale) {
		shim::screen_offset.y = 0;
	}

	if (shim::opengl) {
		glViewport_ptr(0, 0, orig_size.w, orig_size.h);
		PRINT_GL_ERROR("glViewport\n");
	}
#ifdef _WIN32
	else {
		D3DVIEWPORT9 viewport = { 0, 0, (DWORD)orig_size.w, (DWORD)orig_size.h, 0.0f, 1.0f };
		shim::d3d_device->SetViewport(&viewport);
	}
#endif

	scissor_x = shim::screen_offset.x;
	scissor_y = shim::screen_offset.y;
	scissor_w = MIN(orig_size.w, int(shim::screen_size.w*shim::scale));
	scissor_h = MIN(orig_size.h, int(shim::screen_size.h*shim::scale));

	real_set_scissor(scissor_x, scissor_y, scissor_w, scissor_h);
	
	set_default_projection(shim::real_screen_size, shim::screen_offset, shim::scale);
	update_projection();

	if (shim::screen_size != last_gui_size) {
		last_gui_size = shim::screen_size;
		for (size_t i = 0; i < shim::guis.size(); i++) {
			if (shim::guis[i]->gui) {
				shim::guis[i]->resize(shim::screen_size);
			}
		}
	}
}

void draw_guis()
{
	if (shim::guis.size() == 0) {
		return;
	}

	gui::GUI *top = shim::guis.back();

	if (top->is_fullscreen()) {
		top->pre_draw();
		top->draw_back();
		top->draw();
		top->draw_fore();
	}
	else {
		for (size_t i = 0; i < shim::guis.size(); i++) {
			shim::guis[i]->pre_draw();
			shim::guis[i]->draw_back();
			shim::guis[i]->draw();
			shim::guis[i]->draw_fore();
		}
	}
}

void draw_notifications()
{
	if (notifications.size() > 0) {
		int now = SDL_GetTicks();
		int elapsed = now - notification_start_time;
		if (elapsed > shim::notification_duration) {
			elapsed = shim::notification_duration;
		}

		const util::Point<int> pad(int(shim::screen_size.w * 0.05f), int(shim::screen_size.h * 0.05f));

		std::string text = notifications[0];
		int w = shim::font->get_text_width(text);
		int h = shim::font->get_height();
		int x = shim::screen_size.w - w - pad.x;
		const int max_y = pad.y;
		int y = max_y;

		if (elapsed >= (shim::notification_duration-shim::notification_fade_duration)) {
			float p = (elapsed - (shim::notification_duration-shim::notification_fade_duration)) / (float)shim::notification_fade_duration;
			//p = p * p;
			y -= int(pad.y * p);
		}
		else {
			y += (SDL_GetTicks() / 500) % 2 == 0 ? 0 : 1;
		}

		int alpha;

		if (elapsed < shim::notification_fade_duration) {
			float p = elapsed / (float)shim::notification_fade_duration;
			//p = p * p;
			alpha = int(255 * p);
		}
		else if (elapsed >= (shim::notification_duration-shim::notification_fade_duration)) {
			float p = (elapsed-(shim::notification_duration-shim::notification_fade_duration)) / (float)shim::notification_fade_duration;
			//p = p * p;
			alpha = 255 - int(255 * p);
		}
		else {
			alpha = 255;
		}

		SDL_Colour colour;
		colour.r = (shim::black.r * alpha) / 255;
		colour.g = (shim::black.g * alpha) / 255;
		colour.b = (shim::black.b * alpha) / 255;
		colour.a = alpha;

		if (shim::font->get_text_width(text) > shim::screen_size.w * 0.9f) {
			h += shim::font->get_height();
            size_t space = text.length()-1;

			do {
				space = text.rfind(' ', space);
				if (space > 0) {
					space--;
				}
			} while (shim::font->get_text_width(text.substr(0, space+1)) > shim::screen_size.w*0.9f);

			std::string line1 = text.substr(0, space+1);
			std::string line2 = text.substr(space+2);

			w = MAX(shim::font->get_text_width(line1), shim::font->get_text_width(line2));
			x = shim::screen_size.w - w - pad.x;

			draw_filled_rectangle(colour, {x - 3.0f, y - 3.0f}, {w + 6.0f, h + 6.0f});
			colour.r = colour.g = colour.b = alpha;

			fancy_draw(colour, line1, {x, y});
			fancy_draw(colour, line2, {x, y+shim::font->get_height()});
		}
		else {
			draw_filled_rectangle(colour, {x - 3.0f, y - 3.0f}, {w + 6.0f, h + 6.0f});
			colour.r = colour.g = colour.b = alpha;
			fancy_draw(colour, text, {x, y});
		}

		if (elapsed >= shim::notification_duration) {
			next_notification();
		}
	}
}

void apply_screen_shake()
{
	if (SDL_GetTicks() < screen_shake_end) {
		if (shim::using_screen_shake == false) {
			shim::using_screen_shake = true;
			gfx::get_matrices(screen_shake_mv, screen_shake_p);
		}
		float x = float(util::rand(0, 2000)) / 1000.0f;
		float y = float(util::rand(0, 2000)) / 1000.0f;
		x -= 1.0f;
		y -= 1.0f;
		shim::screen_shake_save = util::Point<float>(x*screen_shake_amount, y*screen_shake_amount);
		glm::mat4 mv = glm::translate(screen_shake_mv, glm::vec3(shim::screen_shake_save.x, shim::screen_shake_save.y, 0.0f));
		gfx::set_matrices(mv, screen_shake_p);
		gfx::update_projection();
	}
	else if (shim::using_screen_shake) {
		shim::using_screen_shake = false;
		gfx::set_matrices(screen_shake_mv, screen_shake_p);
		gfx::update_projection();
	}
}

void flip()
{
#if !defined IOS && !defined ANDROID
	// Handle screen orientation changes on desktop
	if (internal::gfx_context.fullscreen && internal::gfx_context.restarting == false) {
		int flags = SDL_GetWindowFlags(internal::gfx_context.window);
		if (flags & SDL_WINDOW_INPUT_FOCUS) {
			SDL_DisplayMode m;
			if (internal::My_SDL_GetCurrentDisplayMode(shim::adapter, &m) == 0) {
				if (last_screen_mode.w > 0 && (m.w != last_screen_mode.w || m.h != last_screen_mode.h)) {
					int w = m.w;
					int h = m.h;
					// If the screen was rotated 90/270 degrees, try to find an exact inverse of the current mode and use that if possible
					if ((w > h) != (shim::real_screen_size.w > shim::real_screen_size.h)) {
						std::vector< util::Size<int> > modes = get_supported_video_modes();
						for (size_t i = 0; i < modes.size(); i++) {
							if (modes[i].h == shim::real_screen_size.h && modes[i].w == shim::real_screen_size.w) {
								w = modes[i].h;
								h = modes[i].w;
								break;
							}
						}
					}
					if (w != shim::real_screen_size.w || h != shim::real_screen_size.h) {
						char **bak_argv = shim::argv;
						int bak_argc = shim::argc;
						int count = shim::argc + 1;
						if (util::check_args(shim::argc, shim::argv, "+width") <= 0) {
							count += 2;
						}
						if (util::check_args(shim::argc, shim::argv, "+height") <= 0) {
							count += 2;
						}
						char **args = (char **)malloc(count * sizeof(char *));
						int j = 0;
						for (int i = 0; i < shim::argc; i++) {
							if (!strcmp(shim::argv[i], "+width")) {
								// skip an extra for number
								i++;
							}
							else if (!strcmp(shim::argv[i], "+height")) {
								// skip an extra for number
								i++;
							}
							else {
								args[j++] = strdup(shim::argv[i]);
							}
						}
						args[j++] = strdup("+width");
						args[j++] = strdup(util::itos(w).c_str());
						args[j++] = strdup("+height");
						args[j++] = strdup(util::itos(h).c_str());
						args[j++] = strdup("+fullscreen");
						shim::argv = args;
						shim::argc = count;
						restart(scaled_w, scaled_h, force_integer_scaling, w, h);
						for (int i = 0; i < count; i++) {
							free(args[i]);
						}
						free(args);
						shim::argv = bak_argv;
						shim::argc = bak_argc;
					}
				}
				last_screen_mode = {m.w, m.h};
			}
		}
	}
#endif

#ifdef _WIN32
	if ((shim::opengl || internal::gfx_context.d3d_lost == false) && internal::gfx_context.restarting == false)
#else
	if (internal::gfx_context.restarting == false)
#endif
	{
		if (show_fps && shim::font != 0) {
			shim::font->draw(shim::white, util::itos(fps), {1.0f, 0.0f});
			shim::font->draw(shim::white, util::itos(fps), {0.0f, 1.0f});
			shim::font->draw(shim::white, util::itos(fps), {3.0f, 0.0f});
			shim::font->draw(shim::white, util::itos(fps), {0.0f, 3.0f});
			shim::font->draw(shim::white, util::itos(fps), {1.0f, 1.0f});
			shim::font->draw(shim::white, util::itos(fps), {3.0f, 3.0f});
			shim::font->draw(shim::white, util::itos(fps), {1.0f, 3.0f});
			shim::font->draw(shim::white, util::itos(fps), {3.0f, 1.0f});
			shim::font->draw(shim::white, util::itos(fps), {2.0f, 2.0f});
		}
	}

	if (shim::opengl) {
		SDL_GL_SwapWindow(internal::gfx_context.window);
	}
#ifdef _WIN32
	else {
		bool begin_scene = true;

		if (internal::gfx_context.d3d_lost) {
			util::infomsg("D3D device is lost.\n");

			bool go = true;
			// Not totally sure why this is here. I _think_ it's for mode changes to make it faster, but it causes some lost devices to be missed.
			/*
			SDL_DisplayMode m;
			if (internal::gfx_context.fullscreen) {
				if (internal::My_SDL_GetCurrentDisplayMode(shim::adapter, &m) == 0) {
					if (m.w != shim::real_screen_size.w || m.h != shim::real_screen_size.h) {
						go = false;

						begin_scene = false;
					}
				}
			}
			*/

			if (go) {
				HRESULT hr = shim::d3d_device->TestCooperativeLevel();
				if (hr == D3DERR_DEVICENOTRESET) {
					internal::handle_lost_device(true, true);

					fill_d3d_pp(shim::real_screen_size.w, shim::real_screen_size.h);

					hr = shim::d3d_device->Reset(&d3d_pp);
					if (hr != D3D_OK) {
						util::infomsg("Device couldn't be reset!\n");
					}
					else {
						internal::gfx_context.d3d_lost = false;

						internal::handle_found_device(true, true);
					}

					begin_scene = false;
				}
			}
		}
		else {
			shim::d3d_device->EndScene();

			HRESULT hr = shim::d3d_device->Present(0, 0, internal::gfx_context.hwnd, 0);

			if (hr == D3DERR_DEVICELOST) {
				util::infomsg("D3D device lost.\n");
				internal::gfx_context.d3d_lost = true;
				begin_scene = false;
			}
		}

		if (begin_scene) {
			shim::d3d_device->BeginScene();
		}
	}
#endif

	apply_screen_shake();

	if (curr_font_size != shim::font_size && internal::gfx_context.inited == true) {
		curr_font_size = shim::font_size;
		destroy_fonts();
		load_fonts();
	}

	if (total_frames == 0) {
		fps_start = SDL_GetTicks();
	}
	total_frames++;
	if (total_frames > 100) {
		fps = int(total_frames / ((SDL_GetTicks() - fps_start) / 1000.0f));
		total_frames = 0;
		fps_start = SDL_GetTicks();
	}

	set_custom_mouse_cursor();
}

void clear(SDL_Colour colour)
{
	if (internal::gfx_context.target_image == 0) {
		if (shim::opengl) {
			glDisable_ptr(GL_SCISSOR_TEST);
		}
#ifdef _WIN32
		else {
			shim::d3d_device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		}
#endif
	}

	if (shim::opengl) {
		glClearColor_ptr(colour.r/255.0f, colour.g/255.0f, colour.b/255.0f, colour.a/255.0f);
		PRINT_GL_ERROR("glClearColor\n");
		glClear_ptr(GL_COLOR_BUFFER_BIT);
		PRINT_GL_ERROR("glClear_ptr(GL_COLOR_BUFFER_BIT)\n");
	}
#ifdef _WIN32
	else {
		shim::d3d_device->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_RGBA(colour.r, colour.g, colour.b, colour.a), 0.0f, 0);
	}
#endif

	if (internal::gfx_context.target_image == 0) {
		real_set_scissor(scissor_x, scissor_y, scissor_w, scissor_h);
	}
}

void clear_depth_buffer(float value)
{
	if (shim::opengl) {
#if !defined ANDROID && !defined IOS
		if (glClearDepthf_ptr == 0) {
			glClearDepth_ptr(value);
		}
		else
#endif
		{
			glClearDepthf_ptr(value);
		}
		PRINT_GL_ERROR("glClearDepthf\n");
		glClear_ptr(GL_DEPTH_BUFFER_BIT);
		PRINT_GL_ERROR("glClear_ptr(GL_DEPTH_BUFFER_BIT)\n");
	}
#ifdef _WIN32
	else {
		shim::d3d_device->Clear(0, 0, D3DCLEAR_ZBUFFER, 0, value, 0);
	}
#endif
}

void clear_stencil_buffer(int value)
{
	if (::create_stencil_buffer == false) {
		return;
	}

	if (shim::opengl) {
		glClearStencil_ptr(value);
		PRINT_GL_ERROR("glClearStencil\n");
		glClear_ptr(GL_STENCIL_BUFFER_BIT);
		PRINT_GL_ERROR("glClear_ptr(GL_STENCIL_BUFFER_BIT)\n");
	}
#ifdef _WIN32
	else {
		shim::d3d_device->Clear(0, 0, D3DCLEAR_STENCIL, 0, 0, value);
	}
#endif
}

void clear_buffers()
{
	clear(shim::black);
	enable_depth_write(true);
	clear_depth_buffer(1.0f);
	enable_depth_write(depth_write_enabled);
	clear_stencil_buffer(0);
}

void draw_9patch_tinted(SDL_Colour tint, Image *image, util::Point<float> dest_position, util::Size<int> dest_size)
{
	float w = image->size.w;
	float h = image->size.h;
	float sz_x = image->size.w / 3;
	float sz_y = image->size.h / 3;
	util::Size<int> dim(sz_x, sz_y);

	image->start_batch();

	// Corners
	image->draw_region_tinted(tint, {0, 0}, dim, dest_position); // top left
	image->draw_region_tinted(tint, {w-sz_x, 0}, dim, {dest_position.x+dest_size.w-sz_x, dest_position.y}); // top right
	image->draw_region_tinted(tint, {w-sz_x, h-sz_y}, dim, dest_position+dest_size-dim); // bottom right
	image->draw_region_tinted(tint, {0, h-sz_y}, dim, {dest_position.x, dest_position.y+dest_size.h-sz_y}); // bottom left

	// Sides
	image->stretch_region_tinted(tint, {sz_x, 0.0f}, dim, {dest_position.x+sz_x, dest_position.y}, {int(dest_size.w-sz_x*2), int(sz_y)}); // top
	image->stretch_region_tinted(tint, {0.0f, sz_y}, dim, {dest_position.x, dest_position.y+sz_y}, {int(sz_x), int(dest_size.h-sz_y*2)}); // left
	image->stretch_region_tinted(tint, {w-sz_x, sz_y}, dim, {dest_position.x+dest_size.w-sz_x, dest_position.y+sz_y}, {int(sz_x), int(dest_size.h-sz_y*2)}); // right
	image->stretch_region_tinted(tint, {sz_x, h-sz_y}, dim, {dest_position.x+sz_x, dest_position.y+dest_size.h-sz_y}, {int(dest_size.w-sz_x*2), int(sz_y)}); // bottom

	// Middle
	image->stretch_region_tinted(tint, {sz_x, sz_y}, dim, dest_position+dim, dest_size-dim*2);

	image->end_batch();
}

void draw_9patch(Image *image, util::Point<int> dest_position, util::Size<int> dest_size)
{
	draw_9patch_tinted(shim::white, image, dest_position, dest_size);
}

void reset_fancy_draw()
{
	fancy_draw_start = SDL_GetTicks();
}

void fancy_draw(SDL_Colour colour, std::string text, util::Point<int> position)
{
	Uint32 t = (SDL_GetTicks() - fancy_draw_start) % 2000;

	int count = (int)text.length();

	std::string curr_s = "";

	//if (t < 1000 || count < 2) {
		shim::font->draw(colour, text, position);
		/*
	}
	else {
		t = t - 1000;

		for (int i = 0; i < util::utf8_len(text); i++) {
			float section = 1000.0f / (count - i + 1);
			float p = t / section;
			if (p > 2.0f) {
				p = 2.0f;
			}
			if (p >= 1.0f) {
				p = p - 1.0f;
			}
			else {
				p = 1.0f - p;
			}
			//p = p * p;
			float x = float(shim::font->get_text_width(curr_s));
			x -= float(shim::font->get_text_width("")); // fix broken fonts that return non zero values here
			float dx = x * p;
			std::string c = util::utf8_substr(text, i, 1);
			shim::font->draw(colour, c, {position.x + dx, (float)position.y});
			curr_s += c;
		}
	}
	*/
}

void add_notification(std::string text)
{
	if (notifications.size() == 0) {
		notification_start_time = SDL_GetTicks();
	}
	notifications.push_back(text);
}

std::string get_current_notification()
{
	if (notifications.size() == 0) {
		return "";
	}
	else {
		return notifications[0];
	}
}

void cancel_current_notification()
{
	next_notification();
}

void cancel_all_notifications()
{
	notifications.clear();
}

int load_palette(std::string name, SDL_Colour *out, int out_size)
{
	name = "gfx/palettes/" + name;

	int sz;
	SDL_RWops *file = util::open_file(name, &sz);

	char line[1000];
	int count = 0;

	util::SDL_fgets(file, line, 1000);
	if (strncmp(line, "GIMP Palette", 12)) {
		SDL_RWclose(file);
		throw util::LoadError("not a GIMP palette: " + name);
	}
	count += strlen(line);

	int line_count = 1;
	int colour_count = 0;

	while (count < sz && colour_count < out_size && util::SDL_fgets(file, line, 1000) != 0) {
		count += strlen(line);
		line_count++;
		char *p = line;
		while (*p != 0 && isspace(*p)) p++;
		// Skip comments
		if (*p == '#') {
			continue;
		}
		int red, green, blue;
		if (sscanf(line, "%d %d %d", &red, &green, &blue) == 3) {
			out[colour_count].r = red;
			out[colour_count].g = green;
			out[colour_count].b = blue;
			out[colour_count].a = 255;
			colour_count++;
			if (colour_count >= out_size) {
				break;
			}
		}
		else {
			util::infomsg("Syntax error on line %d of %s.\n", line_count, name.c_str());
		}
	}

	util::close_file(file);

	return colour_count;
}

int load_default_palette()
{
	return load_palette("palette.gpl", shim::palette);
}

void show_mouse_cursor(bool show)
{
	mouse_cursor_shown = show;
}

void set_scissor(int x, int y, int w, int h)
{
	int sx, sy;
	float scale;
	if (internal::gfx_context.target_image == internal::gfx_context.work_image || internal::gfx_context.target_image == 0 || (default_modelview == modelview && default_proj == proj)) {
		sx = scissor_x;
		sy = scissor_y;
		scale = shim::scale;
	}
	else {
		sx = sy = 0;
		scale = 1.0f;
	}
	real_set_scissor(int(sx + x * scale), int(sy + y * scale), int(w * scale), int(h * scale));
}

void unset_scissor()
{
	int sx, sy, sw, sh;
	if (internal::gfx_context.target_image == internal::gfx_context.work_image || internal::gfx_context.target_image == 0 || (default_modelview == modelview && default_proj == proj)) {
		sx = scissor_x;
		sy = scissor_y;
		sw = scissor_w;
		sh = scissor_h;
		real_set_scissor(sx, sy, sw, sh);
	}
	else {
		/*
		sx = sy = 0;
		sw = internal::gfx_context.target_image->size.w;
		sh = internal::gfx_context.target_image->size.h;
		*/
		if (shim::opengl) {
			glDisable_ptr(GL_SCISSOR_TEST);
		}
#ifdef _WIN32
		else {
			shim::d3d_device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		}
#endif
	}
}

void enable_depth_test(bool onoff)
{
	if (shim::opengl) {
		if (onoff) {
			glEnable_ptr(GL_DEPTH_TEST);
			PRINT_GL_ERROR("glEnable_ptr(GL_DEPTH_TEST)\n");
		}
		else {
			glDisable_ptr(GL_DEPTH_TEST);
			PRINT_GL_ERROR("glDisable_ptr(GL_DEPTH_TEST)\n");
		}
	}
#ifdef _WIN32
	else {
		shim::d3d_device->SetRenderState(D3DRS_ZENABLE, onoff ? D3DZB_TRUE : D3DZB_FALSE);
	}
#endif
}

void enable_depth_write(bool onoff)
{
	if (shim::opengl) {
		if (onoff) {
			glDepthMask_ptr(GL_TRUE);
			PRINT_GL_ERROR("glDepthMask\n");
		}
		else {
			glDepthMask_ptr(GL_FALSE);
			PRINT_GL_ERROR("glDepthMask\n");
		}
	}
#ifdef _WIN32
	else {
		shim::d3d_device->SetRenderState(D3DRS_ZWRITEENABLE, onoff);
	}
#endif

	depth_write_enabled = onoff;
}

void set_depth_mode(Compare_Func func)
{
	if (shim::opengl) {
		glDepthFunc_ptr(shim_compare_to_gl(func));
		PRINT_GL_ERROR("glDepthFunc\n");
	}
#ifdef _WIN32
	else {
		shim::d3d_device->SetRenderState(D3DRS_ZFUNC, shim_compare_to_d3d(func));
	}
#endif
}

void enable_stencil(bool onoff)
{
	if (shim::opengl) {
		if (onoff) {
			glEnable_ptr(GL_STENCIL_TEST);
			PRINT_GL_ERROR("glEnable_ptr(GL_STENCIL_TEST)\n");
		}
		else {
			glDisable_ptr(GL_STENCIL_TEST);
			PRINT_GL_ERROR("glEDisble_ptr(GL_STENCIL_TEST)\n");
		}
	}
#ifdef _WIN32
	else {
		shim::d3d_device->SetRenderState(D3DRS_STENCILENABLE, onoff);
	}
#endif
}

void enable_two_sided_stencil(bool onoff)
{
	two_sided_stencil = onoff;

	if (shim::opengl) {
		// nothing?
	}
#ifdef _WIN32
	else {
		shim::d3d_device->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, onoff);
	}
#endif
}

void set_stencil_mode(Compare_Func func, Stencil_Op fail, Stencil_Op zfail, Stencil_Op pass, int reference, int mask)
{
	if (shim::opengl) {
#ifdef ANDROID
		two_sided_stencil = false;
#endif
		if (two_sided_stencil == false) {
			glStencilFunc_ptr(shim_compare_to_gl(func), reference, mask);
			PRINT_GL_ERROR("glStencilFunc\n");
			glStencilOp_ptr(
				shim_stencilop_to_gl(fail),
				shim_stencilop_to_gl(zfail),
				shim_stencilop_to_gl(pass)
			);
			PRINT_GL_ERROR("glStencilOp\n");
		}
#ifndef ANDROID
		else {
			glStencilFuncSeparate_ptr(GL_FRONT, shim_compare_to_gl(func), reference, mask);
			PRINT_GL_ERROR("glStencilFuncSeparate\n");
			glStencilOpSeparate_ptr(
				GL_FRONT,
				shim_stencilop_to_gl(fail),
				shim_stencilop_to_gl(zfail),
				shim_stencilop_to_gl(pass)
			);
			PRINT_GL_ERROR("glStencilOpSeparate\n");
		}
#endif
	}
#ifdef _WIN32
	else {
		shim::d3d_device->SetRenderState(D3DRS_STENCILFUNC, shim_compare_to_d3d(func));
		shim::d3d_device->SetRenderState(D3DRS_STENCILFAIL, shim_stencilop_to_d3d(fail));
		shim::d3d_device->SetRenderState(D3DRS_STENCILZFAIL, shim_stencilop_to_d3d(zfail));
		shim::d3d_device->SetRenderState(D3DRS_STENCILPASS, shim_stencilop_to_d3d(pass));
		shim::d3d_device->SetRenderState(D3DRS_STENCILREF, reference);
		shim::d3d_device->SetRenderState(D3DRS_STENCILMASK, mask);
	}
#endif
}

void set_stencil_mode_backfaces(Compare_Func func, Stencil_Op fail, Stencil_Op zfail, Stencil_Op pass, int reference, int mask)
{
	if (shim::opengl) {
#ifdef ANDROID
		two_sided_stencil = false;
#endif
		if (two_sided_stencil == false) {
			glStencilFunc_ptr(shim_compare_to_gl(func), reference, mask);
			PRINT_GL_ERROR("glStencilFunc\n");
			glStencilOp_ptr(
				shim_stencilop_to_gl(fail),
				shim_stencilop_to_gl(zfail),
				shim_stencilop_to_gl(pass)
			);
			PRINT_GL_ERROR("glStencilOp\n");
		}
#ifndef ANDROID
		else {
			glStencilFuncSeparate_ptr(GL_BACK, shim_compare_to_gl(func), reference, mask);
			PRINT_GL_ERROR("glStencilFuncSeparate\n");
			glStencilOpSeparate_ptr(
				GL_BACK,
				shim_stencilop_to_gl(fail),
				shim_stencilop_to_gl(zfail),
				shim_stencilop_to_gl(pass)
			);
			PRINT_GL_ERROR("glStencilOpSeparate\n");
		}
#endif
	}
#ifdef _WIN32
	else {
		shim::d3d_device->SetRenderState(D3DRS_CCW_STENCILFUNC, shim_compare_to_d3d(func));
		shim::d3d_device->SetRenderState(D3DRS_CCW_STENCILFAIL, shim_stencilop_to_d3d(fail));
		shim::d3d_device->SetRenderState(D3DRS_CCW_STENCILZFAIL, shim_stencilop_to_d3d(zfail));
		shim::d3d_device->SetRenderState(D3DRS_CCW_STENCILPASS, shim_stencilop_to_d3d(pass));
		shim::d3d_device->SetRenderState(D3DRS_STENCILREF, reference);
		shim::d3d_device->SetRenderState(D3DRS_STENCILMASK, mask);
	}
#endif
}

void set_cull_mode(Faces cull)
{
	if (shim::opengl) {
		if (cull == NO_FACE) {
			glDisable_ptr(GL_CULL_FACE);
			PRINT_GL_ERROR("glDisable_ptr(GL_CULL_FACE)\n");
		}
		else {
			glEnable_ptr(GL_CULL_FACE);
			PRINT_GL_ERROR("glEnable_ptr(GL_CULL_FACE)\n");

			if (cull == FRONT_FACE) {
				glCullFace_ptr(GL_FRONT);
				PRINT_GL_ERROR("glCullFace\n");
			}
			else if (cull == BACK_FACE) {
				glCullFace_ptr(GL_BACK);
				PRINT_GL_ERROR("glCullFace\n");
			}
		}
	}
#ifdef _WIN32
	else {
		if (cull == FRONT_FACE) {
			shim::d3d_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		}
		else if (cull == BACK_FACE) {
			shim::d3d_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		}
		else {
			shim::d3d_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		}
	}
#endif
}

void enable_blending(bool onoff)
{
	blending_enabled = onoff;

	if (shim::opengl) {
		if (onoff) {
			glEnable_ptr(GL_BLEND);
			PRINT_GL_ERROR("glEnable_ptr(GL_BLEND)\n");
		}
		else {
			glDisable_ptr(GL_BLEND);
			PRINT_GL_ERROR("glDisable_ptr(GL_BLEND)\n");
		}
	}
#ifdef _WIN32
	else {
		shim::d3d_device->SetRenderState(D3DRS_ALPHABLENDENABLE, onoff);
	}
#endif
}

void set_blend_mode(Blend_Mode source, Blend_Mode dest)
{
	if (shim::opengl) {
		glBlendFunc_ptr(
			shim_blend_to_gl(source),
			shim_blend_to_gl(dest)
		);
		PRINT_GL_ERROR("glBlendFunc\n");
	}
#ifdef _WIN32
	else {
		shim::d3d_device->SetRenderState(D3DRS_SRCBLEND, shim_blend_to_d3d(source));
		shim::d3d_device->SetRenderState(D3DRS_DESTBLEND, shim_blend_to_d3d(dest));
	}
#endif
}

bool is_blending_enabled()
{
	return blending_enabled;
}

void enable_colour_write(bool onoff)
{
	if (shim::opengl) {
		if (onoff) {
			glColorMask_ptr(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			PRINT_GL_ERROR("glColorMask\n");
		}
		else {
			glColorMask_ptr(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			PRINT_GL_ERROR("glColorMask\n");
		}
	}
#ifdef _WIN32
	else {
		if (onoff) {
			shim::d3d_device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
		}
		else {
			shim::d3d_device->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		}
	}
#endif
}

void set_min_aspect_ratio(float min)
{
	min_aspect_ratio = min;
}

void set_max_aspect_ratio(float max)
{
	max_aspect_ratio = max;
}

util::Size<int> get_desktop_resolution()
{
	util::Size<int> size;
#if defined IOS
	int num = SDL_GetNumDisplayModes(shim::adapter);
	size = {0, 0};
	for (int i = 0; i < num; i++) {
		SDL_DisplayMode mode;
		SDL_GetDisplayMode(shim::adapter, i, &mode);
		if ((mode.w > size.w && mode.h > size.h) || (mode.w == size.w && mode.h > size.h) || (mode.w > size.w && mode.h == size.h)) {
			size = {mode.w, mode.h};
		}
	}
#elif defined __APPLE__
	size = macosx_get_desktop_resolution();
#else
	SDL_DisplayMode mode;
	SDL_GetDesktopDisplayMode(shim::adapter, &mode);
	size = {mode.w, mode.h};
#endif
	return size;
}

std::vector< util::Size<int> > get_supported_video_modes()
{
	std::vector< util::Size<int> > modes;

	int num_modes = SDL_GetNumDisplayModes(shim::adapter);

	SDL_DisplayMode mode;

	int w = -1;
	int h = -1;

	if ((internal::gfx_context.inited || internal::gfx_context.restarting) && internal::gfx_context.fullscreen) {
		w = shim::real_screen_size.w;
		h = shim::real_screen_size.h;
	}
	else if (internal::My_SDL_GetCurrentDisplayMode(shim::adapter, &mode) == 0) {
		w = mode.w;
		h = mode.h;
	}

	for (int i = 0; i < num_modes; i++) {
		SDL_DisplayMode mode;
		if (SDL_GetDisplayMode(shim::adapter, i, &mode) == 0) {
			util::Size<int> sz(mode.w, mode.h);
			if (w > 0 && ((mode.w > mode.h && h > w) || (mode.h > mode.w && w > h))) {
				int tmp = sz.w;
				sz.w = sz.h;
				sz.h = tmp;
			}
			if (std::find(modes.begin(), modes.end(), sz) == modes.end()) {
				modes.push_back(sz);
			}
		}
	}

	return modes;
}

void set_custom_mouse_cursor()
{
// This code works in Dog-O but for some reason not this game on Linux so return
#if defined __linux__
	return;
#endif

#if defined ANDROID || defined RASPBERRYPI || defined IOS
	return;
#else
	if (internal::gfx_context.inited == false) {
		return;
	}

	if (internal::gfx_context.mouse_in_window == false) {
		return;
	}

	if (mouse_cursor_shown) {
		SDL_ShowCursor(1);
		if (use_custom_cursor) {
			SDL_SetCursor(mouse_cursor);
		}
	}
	else {
		SDL_ShowCursor(0);
	}
#endif
}

int get_max_comfortable_scale(util::Size<int> scaled_size)
{
	util::Size<int> mode = get_desktop_resolution();
		
	// Give room for toolbars and decorations
	int w = int(mode.w * 0.75f);
	int h = int(mode.h * 0.75f);

	int nw = w / scaled_size.w;
	int nh = h / scaled_size.h;

	return MIN(nw, nh);
}

bool enable_press_and_hold(bool enable)
{
	press_and_hold_state = enable ? 1 : 0;

#ifdef _WIN32
	// Toggle the press and hold gesture for the given window
	// See: https://msdn.microsoft.com/en-us/library/ms812373.aspx

	// The atom identifier and Tablet PC atom
	ATOM atomID = 0;
	LPCTSTR tabletAtom = "MicrosoftTabletPenServiceProperty";

	// Get the Tablet PC atom ID
	atomID = GlobalAddAtom(tabletAtom);

	// If getting the ID failed, return false
	if (atomID == 0) {
		return false;
	}

	// Enable or disable the press and hold gesture
	if (enable) {
		// Try to enable press and hold gesture by 
		// clearing the window property, return the result
		return RemoveProp(internal::gfx_context.hwnd, tabletAtom) != 0;
	}
	else {
		// Try to disable press and hold gesture by 
		// setting the window property, return the result
		return SetProp(internal::gfx_context.hwnd, tabletAtom, (HANDLE)1) != 0;
	}
#else
	return false;
#endif
}

#ifdef _WIN32
bool is_d3d_lost()
{
	return internal::gfx_context.d3d_lost;
}
#endif

bool is_fullscreen()
{
	return internal::gfx_context.fullscreen || internal::gfx_context.fullscreen_window;
}

bool is_real_fullscreen()
{
	return internal::gfx_context.fullscreen;
}

bool is_fullscreen_window()
{
	return internal::gfx_context.fullscreen_window;
}

void set_minimum_window_size(util::Size<int> size)
{
#ifndef IOS
	if (internal::gfx_context.inited) {
		real_set_minimum_window_size(size);
	}
	minimum_window_size = size;
#endif
}

void set_maximum_window_size(util::Size<int> size)
{
#ifndef IOS
	if (internal::gfx_context.inited) {
		real_set_maximum_window_size(size);
	}
	maximum_window_size = size;
#endif
}

// This doesn't resize the window, but should be called after the window is resized
void resize_window(int width, int height)
{
#ifdef IOS
	SDL_GL_GetDrawableSize(internal::gfx_context.window, &width, &height);
#endif
	
	util::infomsg("Resizing window (%dx%d)...\n", width, height);

	shim::real_screen_size = {width, height};

	internal::handle_lost_device(false);

#ifdef _WIN32
	if (shim::opengl == false) {
		fill_d3d_pp(shim::real_screen_size.w, shim::real_screen_size.h);

		HRESULT hr = shim::d3d_device->Reset(&d3d_pp);

		if (hr != D3D_OK) {
			util::infomsg("Device couldn't be reset/created!\n");
			internal::gfx_context.d3d_lost = true;
			return;
		}
		else {
			internal::gfx_context.d3d_lost = false;
			internal::handle_found_device(false);
		}
	}
	else
#endif
	{
		internal::handle_found_device(false);
	}
}

gfx::Image *gen_plasma(int seed, float alpha1, float alpha2, SDL_Colour tint)
{
	// set a seed so it's always the same
	util::srand(seed);

	// initial size
	int w = 16;
	int h = 8;
	// double the size reps times
	int reps = 3;
	std::vector< std::vector< int > > v(h, std::vector<int>(w, 0));
	// start cell random
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			int r = util::rand(0, 255);
			v[y][x] = r;
		}
	}
	// expand the size, getting an average of surrounding tiles each time
	for (int i = 0; i < reps; i++) {
		int new_w = w * 2;
		int new_h = h * 2;
		std::vector< std::vector<int> > v2(new_h, std::vector<int>(new_w, 0));
		for (int y = 0; y < new_h; y++) {
			for (int x = 0; x < new_w; x++) {
				int y1 = ((y - 1 + new_h) % new_h) / 2;
				int y2 = y / 2;
				int y3 = ((y + 1) % new_h) / 2;
				int x1 = ((x - 1 + new_w) % new_w) / 2;
				int x2 = x / 2;
				int x3 = ((x + 1) % new_w) / 2;
				int val = 0;
				val += v[y1][x1];
				val += v[y1][x2];
				val += v[y1][x3];
				val += v[y2][x1];
				val += v[y2][x2];
				val += v[y2][x3];
				val += v[y3][x1];
				val += v[y3][x2];
				val += v[y3][x3];
				v2[y][x] = val / 9;
			}
		}
		v = v2;
		w = new_w;
		h = new_h;
	}

	// reset the seed
	util::srand((uint32_t)time(NULL));

	Uint8 *data = new Uint8[w*h*4];
	Uint8 *p = data;

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			int val = v[y][x];
			float af = val / 255.0f;
			SDL_Colour colour;
			colour.r = tint.r * af;
			colour.g = tint.g * af;
			colour.b = tint.b * af;
			colour.a = 255 * af;
			*p++ = colour.r;
			*p++ = colour.g;
			*p++ = colour.b;
			*p++ = colour.a;
		}
	}

	gfx::Image *img = new gfx::Image(data, {w, h}, true);

	return img;
}

void screen_shake(float amount, Uint32 length)
{
	screen_shake_amount = amount;
	screen_shake_end = SDL_GetTicks() + length;
}

namespace internal {

GFX_Context gfx_context;

// Returns true if the event should be ignored because it's off the screen after scaling (click/move on black bars)
bool scale_mouse_event(TGUI_Event *event)
{
	// Scale mouse events by the screen scale factor
	if (event->type == TGUI_MOUSE_DOWN || event->type == TGUI_MOUSE_UP || event->type == TGUI_MOUSE_AXIS) {
		if (event->mouse.normalised) {
			int window_w, window_h;
#ifdef IOS
			SDL_GL_GetDrawableSize(internal::gfx_context.window, &window_w, &window_h);
#else
			window_w = shim::real_screen_size.w;
			window_h = shim::real_screen_size.h;
#endif
			event->mouse.x *= window_w;
			event->mouse.y *= window_h;
		}
		event->mouse.x = (event->mouse.x - shim::screen_offset.x) / shim::scale;
		event->mouse.y = (event->mouse.y - shim::screen_offset.y) / shim::scale;
		event->mouse.normalised = false;
		// Due to scaling and offset, mouse events can come in outside of the playable area, skip those
		if (event->mouse.x < 0 || event->mouse.x >= shim::screen_size.w || event->mouse.y < 0 || event->mouse.y >= shim::screen_size.h) {
			return true;
		}
	}

	return false;
}

void handle_lost_device(bool including_opengl, bool force)
{
	if (handled_lost) {
		return;
	}

	if (lost_device_callback) {
		lost_device_callback();
	}

	delete internal::gfx_context.plasma;
	internal::gfx_context.plasma = NULL;

	for (size_t i = 0; i < shim::guis.size(); i++) {
		shim::guis[i]->lost_device();
	}

	Font::release_all();

	Tilemap::release_sheets();

	delete internal::gfx_context.work_image;
	internal::gfx_context.work_image = 0;

#ifdef _WIN32
	if (shim::opengl == false) {
		if (internal::gfx_context.target_image) {
			Image *img = internal::gfx_context.target_image;
			internal::gfx_context.target_image = 0;
			img->release_target();
		}

		if (internal::gfx_context.render_target != 0) {
			util::verbosemsg("handle_lost_device, render_target->Release=%d\n", internal::gfx_context.render_target->Release());
			internal::gfx_context.render_target = 0;
		}

		if (internal::gfx_context.depth_stencil_buffer != 0) {
			util::verbosemsg("handle_lost_device, depth_stencil_buffer->Release=%d\n", internal::gfx_context.depth_stencil_buffer->Release());
			internal::gfx_context.depth_stencil_buffer = 0;
			d3d_device_depth_count--;
		}
		
		shim::d3d_device->SetDepthStencilSurface(0);
	}
#endif

	if (including_opengl || shim::opengl == false) {
		Image::release_all(force);
		Shader::release_all(force);
	}

	Vertex_Cache::instance()->reset();
	
	handled_lost = true;

// FIXME:
	audit();
	Image::audit();
	Shader::audit();
}

void handle_found_device(bool including_opengl, bool force)
{
	try {
#ifdef _WIN32
		if (shim::opengl == false) {
			d3d_create_depth_buffer();

			set_initial_d3d_state();

			if (internal::gfx_context.render_target == 0) {
				if (shim::d3d_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &internal::gfx_context.render_target) != D3D_OK) {
					util::infomsg("GetBackBuffer failed after resize.\n");
				}
			}
		}
#endif

		if (including_opengl || shim::opengl == false) {
			Shader::reload_all(force);

			set_default_shader();

			Image::reload_all(force);
		}

		set_screen_size(shim::real_screen_size);

		Tilemap::reload_sheets();

		internal::recreate_work_image();

		internal::gfx_context.plasma = gen_plasma(util::rand(0, 10000), 1.0f, 0.0f, shim::white);

		for (size_t i = 0; i < shim::guis.size(); i++) {
			shim::guis[i]->found_device();
		}

		if (found_device_callback) {
			found_device_callback();
		}

		handled_lost = false;
	}
	catch (util::Error &) {
		handled_lost = false;
		throw;
	}
}

int My_SDL_GetCurrentDisplayMode(int adapter, SDL_DisplayMode *mode)
{
#ifdef _WIN32
	DEVMODE m;
	memset(&m, 0, sizeof(m));
	m.dmSize = sizeof(m);
	m.dmDriverExtra = 0;
	DISPLAY_DEVICE d;
	memset(&d, 0, sizeof(d));
	d.cb = sizeof(d);
	EnumDisplayDevices(0, shim::adapter, &d, 0);
	if (EnumDisplaySettingsEx(d.DeviceName, ENUM_CURRENT_SETTINGS, &m, EDS_ROTATEDMODE) == 0) {
		return 1;
	}
	mode->w = m.dmPelsWidth;
	mode->h = m.dmPelsHeight;
	mode->refresh_rate = m.dmDisplayFrequency;
	return 0;
#elif defined __APPLE__ && !defined IOS
	util::Size<int> size = macosx_get_desktop_resolution();
	mode->w = size.w;
	mode->h = size.h;
	mode->refresh_rate = 0;
	return 0;
#else
	int ret = SDL_GetCurrentDisplayMode(shim::adapter, mode);
	return ret;
#endif
}

#ifdef _WIN32
/* The following Windows icon and other mouse cursor creation code comes from Allegro, http://liballeg.org */

#define WINDOWS_RGB(r,g,b)  ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

static BITMAPINFO *get_bitmap_info(util::Size<int> size)
{
	BITMAPINFO *bi;
	int i;

	bi = (BITMAPINFO *)new Uint8[sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256];

	ZeroMemory(&bi->bmiHeader, sizeof(BITMAPINFOHEADER));

	bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi->bmiHeader.biBitCount = 32;
	bi->bmiHeader.biPlanes = 1;
	bi->bmiHeader.biWidth = size.w;
	bi->bmiHeader.biHeight = -size.h;
	bi->bmiHeader.biClrUsed = 256;
	bi->bmiHeader.biCompression = BI_RGB;

	for (i = 0; i < 256; i++) {
		bi->bmiColors[i].rgbRed = 0;
		bi->bmiColors[i].rgbGreen = 0;
		bi->bmiColors[i].rgbBlue = 0;
		bi->bmiColors[i].rgbReserved = 0;
	}

	return bi;
}

static void stretch_blit_to_hdc(BYTE *pixels, util::Size<int> size, HDC dc, int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y, int dest_w, int dest_h)
{
	const int bitmap_h = size.h;
	const int bottom_up_src_y = bitmap_h - src_y - src_h;
	BITMAPINFO *bi;

	bi = get_bitmap_info(size);

	if (bottom_up_src_y == 0 && src_x == 0 && src_h != bitmap_h) {
		StretchDIBits(dc, dest_x, dest_h+dest_y-1, dest_w, -dest_h, src_x, bitmap_h - src_y + 1, src_w, -src_h, pixels, bi, DIB_RGB_COLORS, SRCCOPY);
	}
	else {
		StretchDIBits(dc, dest_x, dest_y, dest_w, dest_h, src_x, bottom_up_src_y, src_w, src_h, pixels, bi, DIB_RGB_COLORS, SRCCOPY);
	}

	delete[] bi;
}

HICON win_create_icon(HWND wnd, Uint8 *data, util::Size<int> size, int xfocus, int yfocus, bool is_cursor)
{
	int x, y;
	int sys_sm_cx, sys_sm_cy;
	HDC h_dc;
	HDC h_and_dc;
	HDC h_xor_dc;
	ICONINFO iconinfo;
	HBITMAP and_mask;
	HBITMAP xor_mask;
	HBITMAP hOldAndMaskBitmap;
	HBITMAP hOldXorMaskBitmap;
	HICON icon;

	Uint8 *tmp = new Uint8[size.area() * 4];
	for (y = 0; y < size.h; y++) {
		Uint8 *src = data + y * (size.w * 4);
		Uint8 *dst = tmp + (size.h-y-1) * (size.w * 4); // flip y
		for (x = 0; x < size.w; x++) {
			Uint8 r = *src++;
			Uint8 g = *src++;
			Uint8 b = *src++;
			Uint8 a = *src++;
			*dst++ = b;
			*dst++ = g;
			*dst++ = r;
			*dst++ = a;
		}
	}

	sys_sm_cx = size.w;
	sys_sm_cy = size.h;

	/* Create bitmap */
	h_dc = GetDC(wnd);
	h_xor_dc = CreateCompatibleDC(h_dc);
	h_and_dc = CreateCompatibleDC(h_dc);

	/* Prepare AND (monochrome) and XOR (colour) mask */
	and_mask = CreateBitmap(sys_sm_cx, sys_sm_cy, 1, 1, 0);
	xor_mask = CreateCompatibleBitmap(h_dc, sys_sm_cx, sys_sm_cy);
	hOldAndMaskBitmap = (HBITMAP) SelectObject(h_and_dc, and_mask);
	hOldXorMaskBitmap = (HBITMAP) SelectObject(h_xor_dc, xor_mask);

	/* Create transparent cursor */
	for (y = 0; y < sys_sm_cy; y++) {
		for (x = 0; x < sys_sm_cx; x++) {
			SetPixel(h_and_dc, x, y, WINDOWS_RGB(255, 255, 255));
			SetPixel(h_xor_dc, x, y, WINDOWS_RGB(0, 0, 0));
		}
	}

	stretch_blit_to_hdc((BYTE *)tmp, size, h_xor_dc, 0, 0, size.w, size.h, 0, 0, size.w, size.h);

	/* Make cursor background transparent */
	for (y = 0; y < size.h; y++) {
		Uint8 *p = tmp + y * (size.w * 4);
		for (x = 0; x < size.w; x++) {

			Uint8 b = *p++;
			Uint8 g = *p++;
			Uint8 r = *p++;
			Uint8 a = *p++;

			if (a != 0) {
				/* Don't touch XOR value */
				SetPixel(h_and_dc, x, y, 0);
			}
			else {
				/* No need to touch AND value */
				SetPixel(h_xor_dc, x, y, WINDOWS_RGB(0, 0, 0));
			}
		}
	}

	SelectObject(h_and_dc, hOldAndMaskBitmap);
	SelectObject(h_xor_dc, hOldXorMaskBitmap);
	DeleteDC(h_and_dc);
	DeleteDC(h_xor_dc);
	ReleaseDC(wnd, h_dc);

	iconinfo.fIcon = is_cursor ? false : true;
	iconinfo.xHotspot = xfocus;
	iconinfo.yHotspot = yfocus;
	iconinfo.hbmMask = and_mask;
	iconinfo.hbmColor = xor_mask;

	icon = CreateIconIndirect(&iconinfo);

	DeleteObject(and_mask);
	DeleteObject(xor_mask);

	delete[] tmp;

	return icon;
}
#endif

void recreate_work_image()
{
	if (internal::gfx_context.work_image != 0) {
		delete internal::gfx_context.work_image;
	}
	bool old_create_depth_buffer = gfx::Image::create_depth_buffer;
	gfx::Image::create_depth_buffer = true;
	internal::gfx_context.work_image = new Image(util::Size<int>(shim::real_screen_size.w, shim::real_screen_size.h));
	gfx::Image::create_depth_buffer = old_create_depth_buffer;
}

} // End namespace internal

} // End namespace gfx

} // End namespace noo
