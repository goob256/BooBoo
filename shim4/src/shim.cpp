#include "shim4/audio.h"
#include "shim4/cpa.h"
#include "shim4/devsettings.h"
#include "shim4/error.h"
#include "shim4/gfx.h"
#include "shim4/gui.h"
#include "shim4/image.h"
#include "shim4/input.h"
#include "shim4/json.h"
#include "shim4/mml.h"
#include "shim4/mt.h"
#include "shim4/primitives.h"
#include "shim4/sample.h"
#include "shim4/shim.h"
#include "shim4/sound.h"
#include "shim4/sprite.h"
#include "shim4/translation.h"
#include "shim4/util.h"
#include "shim4/vertex_cache.h"

#include "shim4/internal/audio.h"
#include "shim4/internal/gfx.h"
#include "shim4/internal/shim.h"
#include "shim4/internal/util.h"

#ifdef STEAMWORKS
#include "shim4/steamworks.h"
#endif

using namespace noo;

#if defined IOS || defined ANDROID
static bool app_in_background;
#endif

#if defined IOS || defined ANDROID
static bool adjust_screen_size;
#endif

namespace noo {

namespace shim {

static TGUI_Event *tgui_event;
static input::Focus_Event *focus_event;
static std::vector<TGUI_Event> pushed_events;
static bool waiting_for_fullscreen_change;
static bool waiting_for_resize;
static bool quitting;
static util::Size<int> switch_out_screen_size;
static Uint32 last_mouse_event;
static bool joystick_was_connected;
static bool joy_lb;
static bool joy_rb;

bool opengl;
SDL_Colour palette[256];
int palette_size;
SDL_Colour black;
SDL_Colour white;
SDL_Colour magenta;
SDL_Colour transparent;
SDL_Colour interface_bg;
SDL_Colour interface_highlight;
SDL_Colour interface_text;
SDL_Colour interface_text_shadow;
SDL_Colour interface_edit_fg;
SDL_Colour interface_edit_bg;
std::vector<gui::GUI *> guis;
float scale;
std::string window_title;
std::string organisation_name;
std::string game_name;
gfx::Shader *current_shader;
gfx::Shader *default_shader;
gfx::Shader *model_shader;
gfx::Shader *appear_shader;
int tile_size;
util::Size<int> screen_size;
util::Size<int> real_screen_size;
gfx::Font *font;
int font_size;
bool create_depth_buffer;
bool create_stencil_buffer;
util::Size<int> depth_buffer_size;
util::Point<int> screen_offset;
float black_bar_percent;
float z_add;
//void (*user_render)();
int refresh_rate;
bool hide_window;
int adapter;
util::Point<int> cursor_hotspot;
bool scale_mouse_cursor;
#ifdef _WIN32
IDirect3DDevice9 *d3d_device;
#endif
audio::MML *music;
audio::MML *widget_sfx;
float music_volume;
float sfx_volume;
int samplerate;
int key_l;
int key_r;
int key_u;
int key_d;
int fullscreen_key;
int devsettings_key;
bool convert_directions_to_focus_events;
float joystick_activate_threshold;
float joystick_deactivate_threshold;
bool mouse_button_repeats;
int mouse_button_repeat_max_movement;
bool dpad_below;
bool allow_dpad_below;
bool dpad_enabled;
void (*joystick_disconnect_callback)();
bool force_tablet;
util::CPA *cpa;
int cpa_extra_bytes_after_exe_data;
Uint8 *cpa_pointer_to_data;
int cpa_data_size;
int notification_duration;
int notification_fade_duration;
Uint32 timer_event_id;
int argc;
char **argv;
bool logging;
int logic_rate;
bool use_cwd;
bool log_tags;
#ifdef DEBUG
int error_level = 9999;
#elif defined IOS
int error_level = 3; // let debugmsg hit Xcode console
#else
int error_level = 1;
#endif
#ifdef TVOS
bool pass_menu_to_os;
#endif
util::JSON *shim_json;
int devsettings_num_rows;
int devsettings_max_width;
bool debug;
#ifdef STEAMWORKS
bool steam_init_failed;
void (*steam_overlay_activated_callback)();
#endif
std::vector<util::A_Star::Way_Point> (*get_way_points)(util::Point<int> start);
util::Point<float> screen_shake_save;
bool using_screen_shake;

static void handle_resize(SDL_Event *event)
{
	// Right after recreating the window on Windows we can get window events from the old window
	if (event->window.windowID != gfx::internal::gfx_context.windowid) {
		return;
	}

#if !defined IOS && !defined ANDROID
	if (gfx::internal::gfx_context.fullscreen == false) {
#endif
		gfx::resize_window(event->window.data1, event->window.data2);
#if !defined IOS && !defined ANDROID
	}
#endif
}

// this may run in a different thread :/
static int event_filter(void *userdata, SDL_Event *event)
{
	switch (event->type)
	{
#ifdef IOS
		case SDL_APP_TERMINATING:
			event->type = SDL_QUIT;
			return 1;
		case SDL_APP_LOWMEMORY:
			return 0;
		case SDL_APP_WILLENTERBACKGROUND:
			return 0;
		case SDL_APP_DIDENTERBACKGROUND:
			app_in_background = true;
			return 0;
		case SDL_APP_WILLENTERFOREGROUND:
			adjust_screen_size = true;
			return 0;
		case SDL_APP_DIDENTERFOREGROUND:
			app_in_background = false;
			SDL_SetHint(SDL_HINT_APPLE_TV_CONTROLLER_UI_EVENTS, "0");
			return 0;
#ifdef TVOS
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			if (event->key.keysym.sym == SDLK_MENU && pass_menu_to_os) {
				SDL_SetHint(SDL_HINT_APPLE_TV_CONTROLLER_UI_EVENTS, "1");
			}
			return 1;
#endif
#elif defined ANDROID
		case SDL_APP_WILLENTERBACKGROUND:
			util::internal::flush_log_file();
			return 0;
		case SDL_APP_WILLENTERFOREGROUND:
			return 0;
		case SDL_APP_DIDENTERBACKGROUND:
			app_in_background = true;
			return 0;
		case SDL_APP_DIDENTERFOREGROUND:
			app_in_background = false;
			return 0;
#endif
		default:
			return 1;
	}
}

static bool init_sdl(int sdl_init_flags)
{
	if (SDL_Init(sdl_init_flags) != 0) {
		throw util::Error(util::string_printf("SDL_Init failed: %s.", SDL_GetError()));
		return false;
	}
	
	SDL_SetEventFilter(event_filter, NULL);

#if !defined ANDROID && !defined TVOS
	SDL_GameControllerEventState(SDL_ENABLE);
#endif

	return true;
}

static void load_mml()
{
	widget_sfx = nullptr;

	try {
		widget_sfx = new audio::MML("sfx/widget.mml");
	}
	catch (util::Error &e) {
		util::infomsg(e.error_message + "\n");
	}

	if (widget_sfx != nullptr) {
		widget_sfx->set_pause_with_sfx(false);
	}
}

static void destroy_mml()
{
	delete widget_sfx;
}

bool static_start(int sdl_init_flags)
{
	if (util::basic_start() == false) {
		return false;
	}

	if (sdl_init_flags == 0) {
#if defined ANDROID || defined TVOS
		sdl_init_flags = SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_AUDIO; // we need to be able to shutdown/bring up the joystick system on Android
#else
		sdl_init_flags = SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC;
#endif
	}


	if (init_sdl(sdl_init_flags) == false) {
		return false;
	}

#if !defined ANDROID && IFDEFED_OUT_FOR_NOW
	try {
		if (cpa_pointer_to_data != 0) {
			cpa = new util::CPA(cpa_pointer_to_data, cpa_data_size);
		}
		else if (argv != 0) {
			cpa = new util::CPA(argv[0]);
		}
		else {
			throw util::Error("Catch me!");
		}
	}
	catch (util::Error &e) {
#endif
		try {
			cpa = new util::CPA();
		}
		catch (util::Error &e) {
			util::infomsg(e.error_message + "\n");
		}
#if !defined ANDROID && IFDEFED_OUT_FOR_NOW
	}
#endif

#if defined IOS || defined ANDROID
	app_in_background = false;
#endif

#if defined IOS
	adjust_screen_size = false;
#elif defined ANDROID
	adjust_screen_size = true;
#endif

	// argc/argv should be set before static_start in shim4 (util::static_start uses them)
	//argc = 0;
	//argv = 0;

	tgui_event = 0;
	focus_event = 0;
	pushed_events.clear();
	waiting_for_fullscreen_change = false;
	waiting_for_resize = false;
	black_bar_percent = 0.0f;
	z_add = 0.0f;
	//user_render = 0;
	refresh_rate = 0;
	music = 0;
	widget_sfx = 0;
	cpa_extra_bytes_after_exe_data = 0;
	cpa_pointer_to_data = 0;
	cpa_data_size = 0;
	timer_event_id = (Uint32)-1;
	quitting = false;
	font = 0;
	font_size = 32;
	adapter = 0;
	cursor_hotspot = {0, 0};
	guis.clear();
	last_mouse_event = SDL_GetTicks();
	joystick_was_connected = false;
	joystick_disconnect_callback = nullptr;
	
	switch_out_screen_size = {-1, -1};

	try {
		shim_json = new util::JSON("shim4.json");
	}
	catch (util::Error &e) {
		// Use a default file
		std::string json = "{}";
		SDL_RWops *file = SDL_RWFromMem((void *)json.c_str(), 2);
		assert(file);
		shim_json = new util::JSON(file);
		SDL_RWclose(file);
	}

	util::JSON::Node *root = shim_json->get_root();

	opengl = root->get_nested_bool("shim>gfx>opengl", &opengl, false, true, true);
	black.r = root->get_nested_byte("shim>gfx>colours>black.r", &black.r, 0);
	black.g = root->get_nested_byte("shim>gfx>colours>black.g", &black.g, 0);
	black.b = root->get_nested_byte("shim>gfx>colours>black.b", &black.b, 0);
	black.a = root->get_nested_byte("shim>gfx>colours>black.a", &black.a, 255);
	white.r = root->get_nested_byte("shim>gfx>colours>white.r", &white.r, 255);
	white.g = root->get_nested_byte("shim>gfx>colours>white.g", &white.g, 255);
	white.b = root->get_nested_byte("shim>gfx>colours>white.b", &white.b, 255);
	white.a = root->get_nested_byte("shim>gfx>colours>white.a", &white.a, 255);
	magenta.r = root->get_nested_byte("shim>gfx>colours>magenta.r", &magenta.r, 255);
	magenta.g = root->get_nested_byte("shim>gfx>colours>magenta.g", &magenta.g, 0);
	magenta.b = root->get_nested_byte("shim>gfx>colours>magenta.b", &magenta.b, 255);
	magenta.a = root->get_nested_byte("shim>gfx>colours>magenta.a", &magenta.a, 255);
	transparent.r = root->get_nested_byte("shim>gfx>colours>transparent.r", &transparent.r, 0);
	transparent.g = root->get_nested_byte("shim>gfx>colours>transparent.g", &transparent.g, 0);
	transparent.b = root->get_nested_byte("shim>gfx>colours>transparent.b", &transparent.b, 0);
	transparent.a = root->get_nested_byte("shim>gfx>colours>transparent.a", &transparent.a, 0);
	interface_bg.r = root->get_nested_byte("shim>gfx>colours>interface_bg.r", &interface_bg.r, 0);
	interface_bg.g = root->get_nested_byte("shim>gfx>colours>interface_bg.g", &interface_bg.g, 0);
	interface_bg.b = root->get_nested_byte("shim>gfx>colours>interface_bg.b", &interface_bg.b, 0);
	interface_bg.a = root->get_nested_byte("shim>gfx>colours>interface_bg.a", &interface_bg.a, 0);
	interface_highlight.r = root->get_nested_byte("shim>gfx>colours>interface_highlight.r", &interface_highlight.r, 0);
	interface_highlight.g = root->get_nested_byte("shim>gfx>colours>interface_highlight.g", &interface_highlight.g, 0);
	interface_highlight.b = root->get_nested_byte("shim>gfx>colours>interface_highlight.b", &interface_highlight.b, 0);
	interface_highlight.a = root->get_nested_byte("shim>gfx>colours>interface_highlight.a", &interface_highlight.a, 0);
	interface_text.r = root->get_nested_byte("shim>gfx>colours>interface_text.r", &interface_text.r, 0);
	interface_text.g = root->get_nested_byte("shim>gfx>colours>interface_text.g", &interface_text.g, 0);
	interface_text.b = root->get_nested_byte("shim>gfx>colours>interface_text.b", &interface_text.b, 0);
	interface_text.a = root->get_nested_byte("shim>gfx>colours>interface_text.a", &interface_text.a, 0);
	interface_text_shadow.r = root->get_nested_byte("shim>gfx>colours>interface_text_shadow.r", &interface_text_shadow.r, 0);
	interface_text_shadow.g = root->get_nested_byte("shim>gfx>colours>interface_text_shadow.g", &interface_text_shadow.g, 0);
	interface_text_shadow.b = root->get_nested_byte("shim>gfx>colours>interface_text_shadow.b", &interface_text_shadow.b, 0);
	interface_text_shadow.a = root->get_nested_byte("shim>gfx>colours>interface_text_shadow.a", &interface_text_shadow.a, 0);
	interface_edit_fg.r = root->get_nested_byte("shim>gfx>colours>interface_edit_fg.r", &interface_edit_fg.r, 0);
	interface_edit_fg.g = root->get_nested_byte("shim>gfx>colours>interface_edit_fg.g", &interface_edit_fg.g, 0);
	interface_edit_fg.b = root->get_nested_byte("shim>gfx>colours>interface_edit_fg.b", &interface_edit_fg.b, 0);
	interface_edit_fg.a = root->get_nested_byte("shim>gfx>colours>interface_edit_fg.a", &interface_edit_fg.a, 0);
	interface_edit_bg.r = root->get_nested_byte("shim>gfx>colours>interface_edit_bg.r", &interface_edit_bg.r, 0);
	interface_edit_bg.g = root->get_nested_byte("shim>gfx>colours>interface_edit_bg.g", &interface_edit_bg.g, 0);
	interface_edit_bg.b = root->get_nested_byte("shim>gfx>colours>interface_edit_bg.b", &interface_edit_bg.b, 0);
	interface_edit_bg.a = root->get_nested_byte("shim>gfx>colours>interface_edit_bg.a", &interface_edit_bg.a, 0);
	window_title = root->get_nested_string("shim>misc>window_title", &window_title, window_title, true, true);
	organisation_name = root->get_nested_string("shim>misc>organisation_name", &organisation_name, organisation_name, true, true);
	game_name = root->get_nested_string("shim>misc>game_name", &game_name, game_name, true, true);
	create_depth_buffer = root->get_nested_bool("shim>gfx>create_depth_buffer", &create_depth_buffer, false);
	create_stencil_buffer = root->get_nested_bool("shim>gfx>create_stencil_buffer", &create_stencil_buffer, false);
	depth_buffer_size.w = root->get_nested_int("shim>gfx>depth_buffer_size.w", &depth_buffer_size.w, -1);
	depth_buffer_size.h = root->get_nested_int("shim>gfx>depth_buffer_size.h", &depth_buffer_size.h, -1);
	scale_mouse_cursor = root->get_nested_bool("shim>gfx>scale_mouse_cursor", &scale_mouse_cursor, false, true, false);
	hide_window = root->get_nested_bool("shim>gfx>hide_window", &hide_window, false, true, true);
	music_volume = root->get_nested_float("shim>audio>music_volume", &music_volume, 1.0f);
	sfx_volume = root->get_nested_float("shim>audio>sfx_volume", &sfx_volume, 1.0f);
	samplerate = root->get_nested_int("shim>audio>samplerate", &samplerate, 0, true, true);
	// these are for devsettings
	key_l = TGUIK_LEFT;
	key_r = TGUIK_RIGHT;
	key_u = TGUIK_UP;
	key_d = TGUIK_DOWN;
	fullscreen_key = root->get_nested_int("shim>input>fullscreen_key", &fullscreen_key, TGUIK_F11);
	devsettings_key = root->get_nested_int("shim>input>devsettings_key", &devsettings_key, TGUIK_F9);
	convert_directions_to_focus_events = root->get_nested_bool("shim>input>convert_directions_to_focus_events", &convert_directions_to_focus_events, true);
	mouse_button_repeats = root->get_nested_bool("shim>input>mouse_button_repeats", &mouse_button_repeats, true);
	mouse_button_repeat_max_movement = root->get_nested_int("shim>input>mouse_button_repeat_max_movement", &mouse_button_repeat_max_movement, -1); // * shim::scale
	dpad_below = false;
	allow_dpad_below = root->get_nested_bool("shim>input>allow_dpad_below", &allow_dpad_below, true, true, false);
	force_tablet = root->get_nested_bool("shim>input>force_tablet", &force_tablet, false, true, false);
	notification_duration = root->get_nested_int("shim>gfx>notification_duration", &notification_duration, 3000);
	notification_fade_duration = root->get_nested_int("shim>gfx>notification_fade_duration", &notification_fade_duration, 500);
	logging = root->get_nested_bool("shim>misc>logging", &logging, logging, true, true);
	logic_rate = root->get_nested_int("shim>misc>logic_rate", &logic_rate, 60);
	use_cwd = root->get_nested_bool("shim>misc>use_cwd", &use_cwd, false);
	log_tags = root->get_nested_bool("shim>misc>log_tags", &log_tags, true);
	error_level = root->get_nested_int("shim>misc>error_level", &error_level, error_level);
#ifdef TVOS
	pass_menu_to_os = root->get_nested_bool("shim>tvos>pass_menu_to_os", &pass_menu_to_os, false);
#endif
	tile_size = root->get_nested_int("shim>gfx>tile_size", &tile_size, 16);
	devsettings_num_rows = root->get_nested_int("shim>misc>devsettings_num_rows", &devsettings_num_rows, 6);
	devsettings_max_width = root->get_nested_int("shim>misc>devsettings_max_width", &devsettings_max_width, 100);
	cursor_hotspot.x = root->get_nested_int("shim>gfx>cursor_hotspot.x", &shim::cursor_hotspot.x, 0);
	cursor_hotspot.y = root->get_nested_int("shim>gfx>cursor_hotspot.y", &shim::cursor_hotspot.y, 0);
	
	debug = root->get_nested_bool("shim>misc>debug", &debug, false);

#ifdef STEAMWORKS
	steam_overlay_activated_callback = nullptr;
	util::start_steamworks();
#endif

	get_way_points = nullptr;

	return true;
}

bool static_start_all(int sdl_init_flags)
{
	if (static_start(sdl_init_flags) == false) {
		return false;
	}
	if (util::static_start() == false) {
		return false;
	}
	if (audio::static_start() == false) {
		return false;
	}
	if (gfx::static_start() == false) {
		return false;
	}

	return true;
}

bool start()
{
	debug = debug || util::bool_arg(false, shim::argc, shim::argv, "debug");

	force_tablet = force_tablet || util::bool_arg(false, shim::argc, shim::argv, "force-tablet");

	if (timer_event_id == (Uint32)-1) {
		timer_event_id = SDL_RegisterEvents(1);
	}

	// Load resources

	load_mml();

	tgui_event = new TGUI_Event;
	focus_event = new input::Focus_Event;

	return true;
}

bool start_all(int scaled_gfx_w, int scaled_gfx_h, bool force_integer_scaling, int gfx_window_w, int gfx_window_h)
{
	util::start();

	if (audio::start() == false) {
		/*
		delete cpa;
		return false;
		*/
		// should continue muted
	}
	
	start();

	if (gfx::start(scaled_gfx_w, scaled_gfx_h, force_integer_scaling, gfx_window_w, gfx_window_h) == false) {
		audio::end();
		delete cpa;
		throw util::Error(util::string_printf("gfx::start failed."));
		return false;
	}

	if (input::start() == false) {
		gfx::end();
		audio::end();
		delete cpa;
		throw util::Error(util::string_printf("input::start failed."));
		return false;
	}

	return true;
}

void static_end()
{
	SDL_Quit();
}

void static_end_all()
{
	gfx::static_end();
	static_end();
	util::static_end();
}

void end()
{
	for (size_t i = 0; i < guis.size(); i++) {
		delete guis[i];
	}

	destroy_mml();

	delete shim::music;
	shim::music = 0;

	delete cpa;

	delete tgui_event;
	delete focus_event;

	util::infomsg("%d unfreed images.\n", gfx::Image::get_unfreed_count());
}

void end_all()
{
	input::end();

	gfx::end();

	end();

	audio::end();

	util::end();
}

static TGUI_Event *real_handle_tgui_event(TGUI_Event *tgui_event)
{
	if (tgui_event->type == TGUI_UNKNOWN) {
		return tgui_event;
	}

	if (tgui_event->type == TGUI_MOUSE_AXIS || tgui_event->type == TGUI_MOUSE_DOWN || tgui_event->type == TGUI_MOUSE_UP) {
		last_mouse_event = SDL_GetTicks();
		gfx::show_mouse_cursor(true);
		gfx::set_custom_mouse_cursor();
	}
	else {
		Uint32 now = SDL_GetTicks();
		Uint32 elapsed = now - last_mouse_event;
		if (elapsed > 10000 && input::is_joystick_connected()) {
			gfx::show_mouse_cursor(false);
		}
	}

	bool ret = gfx::internal::scale_mouse_event(tgui_event);

	input::handle_event(tgui_event);

	bool is_focus;

	if (convert_directions_to_focus_events) {
		is_focus = input::convert_to_focus_event(tgui_event, focus_event);
	}
	else {
		is_focus = false;
	}

	TGUI_Event *event;
	if (is_focus) {
		event = focus_event;
	}
	else {
		event = tgui_event;
	}

	if (ret) {
		return event;
	}

	if (guis.size() > 0) {
		gui::GUI *noo_gui = guis[guis.size()-1];
		if (noo_gui->is_transitioning_out() == false) {
			noo_gui->handle_event(event);
		}
	}

	if (!gfx::internal::gfx_context.fullscreen && event->type == TGUI_KEY_DOWN && event->keyboard.code == fullscreen_key && event->keyboard.is_repeat == false) {
		if (shim::guis.size() == 0 || dynamic_cast<gui::DevSettings_GUI *>(shim::guis.back()) == NULL) {
			waiting_for_fullscreen_change = true;
			gfx::clear(shim::black);
			gfx::flip();
			gfx::internal::gfx_context.fullscreen_window = !gfx::internal::gfx_context.fullscreen_window;
			SDL_SetWindowFullscreen(gfx::internal::gfx_context.window, gfx::internal::gfx_context.fullscreen_window ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
			gfx::clear(shim::black);
			gfx::flip();
		}
	}

	if (event->type == TGUI_JOY_DOWN && event->joystick.is_repeat == false) {
		if (event->joystick.button == TGUI_B_LB) {
			joy_lb = true;
		}
		else if (event->joystick.button == TGUI_B_RB) {
			joy_rb = true;
		}
	}
	else if (event->type == TGUI_JOY_UP && event->joystick.is_repeat == false) {
		if (event->joystick.button == TGUI_B_LB) {
			joy_lb = false;
		}
		else if (event->joystick.button == TGUI_B_RB) {
			joy_rb = false;
		}
	}

	if ((event->type == TGUI_KEY_DOWN && event->keyboard.code == devsettings_key && event->keyboard.is_repeat == false) || (joy_lb && joy_rb)) {
#ifdef DEBUG
		if (true) {
#else
		if (shim::debug) {
#endif
			if (shim::guis.size() == 0 || dynamic_cast<gui::DevSettings_GUI *>(shim::guis.back()) == NULL) {
				gui::DevSettings_GUI *gui = new gui::DevSettings_GUI();
				shim::guis.push_back(gui);
			}
			// exiting is done in the gui itself now
			/*
			else {
				gui::GUI *gui = shim::guis.back();
				gui::DevSettings_GUI *d = dynamic_cast<gui::DevSettings_GUI *>(gui);
				if (d->is_editing() == false) {
					shim::guis.pop_back();
					delete gui;
				}
			}
			*/
		}
	}

	// Don't pass events to game if DevSettings_GUI is up
	bool found_devsettings = false;
	for (auto g : shim::guis) {
		if (dynamic_cast<gui::DevSettings_GUI *>(g)) {
			found_devsettings = true;
			break;
		}
	}
	if (found_devsettings) {
		event->type = TGUI_UNKNOWN;
	}

	return event;
}

TGUI_Event *handle_event(SDL_Event *sdl_event)
{
#ifdef IOS
	if (sdl_event->type == SDL_WINDOWEVENT && sdl_event->window.event == SDL_WINDOWEVENT_RESIZED) {
#elif defined ANDROID
	if (sdl_event->type == SDL_WINDOWEVENT && (sdl_event->window.event == SDL_WINDOWEVENT_RESIZED || sdl_event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
#else
	if (sdl_event->type == SDL_WINDOWEVENT && sdl_event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
#endif
		waiting_for_fullscreen_change = false;
		handle_resize(sdl_event);
	}
	else if (sdl_event->type == SDL_WINDOWEVENT && sdl_event->window.event == SDL_WINDOWEVENT_ENTER) {
		gfx::internal::gfx_context.mouse_in_window = true;
	}
	else if (sdl_event->type == SDL_WINDOWEVENT && sdl_event->window.event == SDL_WINDOWEVENT_LEAVE) {
		gfx::internal::gfx_context.mouse_in_window = false;
	}
	else if (sdl_event->type == SDL_QUIT) {
		quitting = true;
	}

	if (sdl_event->type == timer_event_id) {
		tgui_event->type = TGUI_TICK;
	}
	else {
#ifdef STEAM_INPUT
		bool go;
		if (shim::steam_init_failed == false) {
			switch (sdl_event->type) {
				case SDL_CONTROLLERBUTTONDOWN:
				case SDL_CONTROLLERBUTTONUP:
				case SDL_CONTROLLERAXISMOTION:
				{
					go = false;
					TGUI_Event e;
					e.type = TGUI_UNKNOWN;
					*tgui_event = e;
					break;
				}
				default:
					go = true;
					break;
			}
		}
		else {
			go = true;
		}
		if (go)
#endif
		{
			*tgui_event = tgui_sdl_convert_event(sdl_event);
		}
	}

	TGUI_Event *event = real_handle_tgui_event(tgui_event);

	return event;
}

TGUI_Event *pop_pushed_event()
{
	if (pushed_events.size() == 0) {
		return 0;
	}
	else {
		*tgui_event = pushed_events[0];
		pushed_events.erase(pushed_events.begin());
		return real_handle_tgui_event(tgui_event);
	}
}

/*
bool event_in_queue(TGUI_Event e)
{
	for (size_t i = 0; i < pushed_events.size(); i++) {
		TGUI_Event &e2 = pushed_events[i];
		if (e.type == e2.type) {
			switch (e.type) {
				case TGUI_UNKNOWN:
				case TGUI_QUIT:
					return true;
				case TGUI_TICK:
					return true;
				case TGUI_KEY_DOWN:
				case TGUI_KEY_UP:
					if (memcmp(&e.keyboard, &e2.keyboard, sizeof(e.keyboard)) == 0) {
						return true;
					}
					break;
				case TGUI_MOUSE_DOWN:
				case TGUI_MOUSE_UP:
				case TGUI_MOUSE_AXIS:
				case TGUI_MOUSE_WHEEL:
					if (memcmp(&e.mouse, &e2.mouse, sizeof(e.mouse)) == 0) {
						return true;
					}
					break;
				case TGUI_JOY_DOWN:
				case TGUI_JOY_UP:
				case TGUI_JOY_AXIS:
					if (memcmp(&e.joystick, &e2.joystick, sizeof(e.joystick)) == 0) {
						return true;
					}
					break;
				case TGUI_FOCUS:
					if (memcmp(&e.focus, &e2.focus, sizeof(e.focus)) == 0) {
						return true;
					}
					break;
				case TGUI_TEXT:
					if (memcpy(&e.text, &e2.text, sizeof(e.text)) == 0) {
						return true;
					}
					break;
			}
		}
	}
	
	SDL_Event events[100];
	int n;
	n = SDL_PeepEvents(&events[0], 100, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
	for (int i = 0; i < n; i++) {
		SDL_Event &e2 = events[i];
		if (e2.type == SDL_KEYDOWN && e.type == TGUI_KEY_DOWN) {
			if (e.keyboard.code == e2.key.keysym.sym && e.keyboard.is_repeat == (e2.key.repeat != 0)) {
				return true;
			}
		}
		if (e2.type == SDL_KEYUP && e.type == TGUI_KEY_UP) {
			if (e.keyboard.code == e2.key.keysym.sym && e.keyboard.is_repeat == (e2.key.repeat != 0)) {
				return true;
			}
		}
		if (e2.type == SDL_MOUSEBUTTONDOWN && e.type == TGUI_MOUSE_DOWN) {
			if (e.mouse.button == e2.button.button && e.mouse.x == e2.button.x && e.mouse.y == e2.button.y) {
				return true;
			}
		}
		if (e2.type == SDL_MOUSEBUTTONUP && e.type == TGUI_MOUSE_UP) {
			if (e.mouse.button == e2.button.button && e.mouse.x == e2.button.x && e.mouse.y == e2.button.y) {
				return true;
			}
		}
		if (e2.type == SDL_MOUSEMOTION && e.type == TGUI_MOUSE_AXIS) {
			if (e.mouse.x == e2.motion.x && e.mouse.y == e2.motion.y) {
				return true;
			}
		}
		if (e2.type == SDL_MOUSEWHEEL && e.type == TGUI_MOUSE_WHEEL) {
			if (e.mouse.x == e2.wheel.x && e.mouse.y == e2.wheel.y) {
				return true;
			}
		}
		if (e2.type == SDL_CONTROLLERBUTTONDOWN && e.type == TGUI_JOY_DOWN) {
			if (e.joystick.id == e2.cbutton.which && e.joystick.button == e2.cbutton.button) {
				return true;
			}
		}
		if (e2.type == SDL_CONTROLLERBUTTONUP && e.type == TGUI_JOY_UP) {
			if (e.joystick.id == e2.jbutton.which && e.joystick.button == e2.jbutton.button) {
				return true;
			}
		}
		if (e2.type == SDL_CONTROLLERAXISMOTION && e.type == TGUI_JOY_AXIS) {
			float v = TGUI4_NORMALISE_JOY_AXIS(e2.caxis.value);
			if (e2.caxis.which == e.joystick.id && e2.caxis.axis == e.joystick.axis && v == e.joystick.value) {
				return true;
			}
		}
		if (e2.type == shim::timer_event_id && e.type == TGUI_TICK) {
			return true;
		}
	}

	return false;
}
*/

bool update()
{
#if defined STEAMWORKS
	if (steam_init_failed == false) {
		SteamAPI_RunCallbacks();
	}
#endif

#if defined IOS || defined ANDROID
	if (app_in_background) {
		return false;
	}
#endif

	if (waiting_for_fullscreen_change) {
		return false;
	}

	if (waiting_for_resize) {
		return false;
	}

	if (quitting) {
		return false;
	}

#if defined IOS || defined ANDROID
	if (adjust_screen_size) {
		adjust_screen_size = false;
		int width, height;
		SDL_GL_GetDrawableSize(gfx::internal::gfx_context.window, &width, &height);
		shim::real_screen_size.w = width;
		shim::real_screen_size.h = height;
		gfx::set_screen_size(shim::real_screen_size);
	}
#endif

	if (gfx::internal::gfx_context.work_image != 0 && gfx::internal::gfx_context.work_image->size != shim::real_screen_size) {
		gfx::internal::recreate_work_image();
	}

	input::update();

	bool jic = input::is_joystick_connected();
	if (jic && joystick_was_connected == false) {
		last_mouse_event = SDL_GetTicks();
	}
	joystick_was_connected = jic; // for use with auto-hiding mouse cursor

	if (guis.size() > 0) {
		gui::GUI *noo_gui = guis[guis.size()-1];
		std::vector<gui::GUI *> other_guis;
		other_guis.insert(other_guis.begin(), guis.begin(), guis.end()-1);
		if (noo_gui->gui && noo_gui->gui->get_focus() == 0) {
			noo_gui->gui->set_focus(noo_gui->focus);
		}
		if (noo_gui->is_transitioning_out() == false) {
			noo_gui->update();
		}
		// Not else if here, the state could change in update()
		if (noo_gui->is_transitioning_out() && noo_gui->is_transition_out_finished()) {
			// update may have pushed other GUIs on the stack, so we can't just erase the last one
			for (size_t i = 0; i < guis.size(); i++) {
				if (guis[i] == noo_gui) {
					guis.erase(guis.begin() + i);
					delete noo_gui;
					break;
				}
			}
		}
		for (size_t i = 0; i < other_guis.size(); i++) {
			noo_gui = other_guis[i];
			if (noo_gui->gui && noo_gui->gui->get_focus() != 0) {
				noo_gui->focus = noo_gui->gui->get_focus();
				noo_gui->gui->set_focus(0);
			}
			if (noo_gui->is_transitioning_out() == false) {
				noo_gui->update_background();
			}
			else if (noo_gui->is_transition_out_finished()) {
				// update may have pushed other GUIs on the stack, so we can't just erase the last one
				for (size_t j = 0; j < guis.size(); j++) {
					if (guis[j] == noo_gui) {
						guis.erase(guis.begin() + j);
						delete noo_gui;
						break;
					}
				}
			}
		}
	}

	return true;
}

void push_event(TGUI_Event event)
{
	pushed_events.push_back(event);
}

namespace internal {

TGUI_Event *handle_tgui_event(TGUI_Event *event)
{
	*tgui_event = *event;
	return real_handle_tgui_event(tgui_event);
}

}

} // End namespace shim

} // End namespace noo
