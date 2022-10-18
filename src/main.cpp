//#define LUA_BENCH
//#define LUA_BENCH2
//#define CPP_BENCH
//#define CPP_BENCH2
//#define DUMP

#include <shim4/shim4.h>

#include "booboo/booboo.h"

#ifdef CPP_BENCH
gfx::Image *grass;
gfx::Image *robot;
#endif

bool load_from_filesystem_set;

bool quit;

booboo::Program prg;

int orig_argc;
char **orig_argv;

std::string extra_args;
std::string extra_args_orig;

#if defined LUA_BENCH || defined LUA_BENCH2
extern "C" {
#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>
}

lua_State *lua_state;
std::vector<gfx::Image *> lua_images;

void dump_lua_stack(lua_State *l)
{
        int i;
        int top = lua_gettop(l);
	char buf[1000];

        snprintf(buf, 1000, "--- stack ---\n");
	printf(buf);
        snprintf(buf, 1000, "top=%u   ...   ", top);
	printf(buf);

        for (i = 1; i <= top; i++) {  /* repeat for each level */
                int t = lua_type(l, i);
                switch (t) {

                case LUA_TSTRING:  /* strings */
                        snprintf(buf, 1000, "`%s'", lua_tostring(l, i));
			printf(buf);
                        break;

                case LUA_TBOOLEAN:  /* booleans */
                        snprintf(buf, 1000, lua_toboolean(l, i) ? "true" : "false");
			printf(buf);
                        break;

                case LUA_TNUMBER:  /* numbers */
                        snprintf(buf, 1000, "%g", lua_tonumber(l, i));
			printf(buf);
                        break;

                case LUA_TTABLE:   /* table */
                        snprintf(buf, 1000, "table");
			printf(buf);
                        break;

                default:  /* other values */
                        snprintf(buf, 1000, "%s", lua_typename(l, t));
			printf(buf);
                        break;

                }
                snprintf(buf, 1000, "  ");  /* put a separator */
        }
        snprintf(buf, 1000, "\n");  /* end the listing */
	printf(buf);

        snprintf(buf, 1000, "-------------\n");
	printf(buf);
}


/*
 * Call a Lua function, leaving the results on the stack.
 */
void call_lua(lua_State* lua_state, const char *func, const char *sig, ...)
{
	va_list vl;
	int narg, nres;  /* number of arguments and results */

	va_start(vl, sig);
	lua_getglobal(lua_state, func);  /* get function */

	if (!lua_isfunction(lua_state, -1)) {
		lua_pop(lua_state, 1);
		return;
	}

	/* push arguments */
	narg = 0;
	while (*sig) {  /* push arguments */
		switch (*sig++) {
			case 'd':  /* double argument */
				lua_pushnumber(lua_state, va_arg(vl, double));
				break;
			case 'b':  /* boolean (int) argument */
				lua_pushboolean(lua_state, va_arg(vl, int));
				break;
			case 'i':  /* int argument */
				lua_pushnumber(lua_state, va_arg(vl, int));
				break;

			case 's':  /* string argument */
				lua_pushstring(lua_state, va_arg(vl, char *));
				break;
			case 'u':  /* userdata argument */
				lua_pushlightuserdata(lua_state, va_arg(vl, void *));
				break;
			case '>':
				goto endwhile;
			default:
				break;
		}
		narg++;
		luaL_checkstack(lua_state, 1, "too many arguments");
	}
endwhile:

	/* do the call */
	nres = strlen(sig);  /* number of expected results */
	if (lua_pcall(lua_state, narg, nres, 0) != 0) {
		dump_lua_stack(lua_state);
	}

	va_end(vl);
}

extern "C" {

static int c_start_primitives(lua_State *stack)
{
	gfx::draw_primitives_start();
	return 0;
}

static int c_end_primitives(lua_State *stack)
{
	gfx::draw_primitives_end();
	return 0;
}

static int c_filled_circle(lua_State *stack)
{
	int r = lua_tonumber(stack, 1);
	int g = lua_tonumber(stack, 2);
	int b = lua_tonumber(stack, 3);
	int a = lua_tonumber(stack, 4);
	float x = lua_tonumber(stack, 5);
	float y = lua_tonumber(stack, 6);
	float radius = lua_tonumber(stack, 7);
	int sections = lua_tonumber(stack, 8);

	SDL_Color c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = a;

	util::Point<float> p(x, y);

	gfx::draw_filled_circle(c, p, radius, sections);

	return 0;
}

static int c_clear(lua_State *stack)
{
	int r = lua_tonumber(stack, 1);
	int g = lua_tonumber(stack, 2);
	int b = lua_tonumber(stack, 3);

	SDL_Color c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = 255;

	gfx::clear(c);

	return 0;
}

static int c_load_image(lua_State *stack)
{
	const char *name = lua_tostring(stack, 1);

	int n = lua_images.size();

	lua_images.push_back(new gfx::Image(name));

	lua_pushnumber(stack, n);

	return 1;
}

static int c_image_size(lua_State *stack)
{
	int n = lua_tonumber(stack, 1);

	lua_pushnumber(stack, lua_images[n]->size.w);
	lua_pushnumber(stack, lua_images[n]->size.h);

	return 2;
}

static int c_image_draw(lua_State *stack)
{
	int n = lua_tonumber(stack, 1);
	int tint_r = lua_tonumber(stack, 2);
	int tint_g = lua_tonumber(stack, 3);
	int tint_b = lua_tonumber(stack, 4);
	int tint_a = lua_tonumber(stack, 5);
	int x = lua_tonumber(stack, 6);
	int y = lua_tonumber(stack, 7);
	int flip_h = lua_tonumber(stack, 8);
	int flip_v = lua_tonumber(stack, 9);

	SDL_Colour c;
	c.r = tint_r;
	c.g = tint_g;
	c.b = tint_b;
	c.a = tint_a;

	int flip = 0;
	if (flip_h) {
		flip |= gfx::Image::FLIP_H;
	}
	if (flip_v) {
		flip |= gfx::Image::FLIP_V;
	}

	lua_images[n]->draw_tinted(c, util::Point<float>(x, y), flip);

	return 0;
}

static int c_image_start(lua_State *stack)
{
	int n = lua_tonumber(stack, 1);

	lua_images[n]->start_batch();

	return 0;
}

static int c_image_end(lua_State *stack)
{
	int n = lua_tonumber(stack, 1);

	lua_images[n]->end_batch();

	return 0;
}

static int c_rand(lua_State *stack)
{
	int min_incl = lua_tonumber(stack, 1);
	int max_incl = lua_tonumber(stack, 2);

	lua_pushnumber(stack, util::rand(min_incl, max_incl));

	return 1;
}

}

void init_lua()
{
	lua_state = luaL_newstate();

	luaL_openlibs(lua_state);

	#define REGISTER_FUNCTION(name) \
		lua_pushcfunction(lua_state, c_ ## name); \
		lua_setglobal(lua_state, #name);

	REGISTER_FUNCTION(clear);
	REGISTER_FUNCTION(start_primitives);
	REGISTER_FUNCTION(end_primitives);
	REGISTER_FUNCTION(filled_circle);
	REGISTER_FUNCTION(load_image);
	REGISTER_FUNCTION(image_size);
	REGISTER_FUNCTION(image_draw);
	REGISTER_FUNCTION(image_start);
	REGISTER_FUNCTION(image_end);
	REGISTER_FUNCTION(rand);

	#undef REGISTER_FUNCION

#ifdef LUA_BENCH
	std::string program = util::load_text_from_filesystem("benchmark.lua");
#else
	std::string program = util::load_text_from_filesystem("benchmark2.lua");
#endif
	luaL_loadstring(lua_state, program.c_str());
	if (lua_pcall(lua_state, 0, 0, 0) != 0) {
		dump_lua_stack(lua_state);
	}
}
#endif

bool start()
{
	// This is basically 16:9 only, with a tiny bit of leeway
	gfx::set_min_aspect_ratio(1.777f);
	gfx::set_max_aspect_ratio(1.777f);

	if (util::bool_arg(false, shim::argc, shim::argv, "logging")) {
		shim::logging = true;
	}

	if (util::bool_arg(false, shim::argc, shim::argv, "dump-images")) {
		gfx::Image::keep_data = true;
		gfx::Image::save_palettes = util::bool_arg(false, shim::argc, shim::argv, "save-palettes");
		gfx::Image::save_rle = true;
		gfx::Image::save_rgba = false;
		gfx::Image::premultiply_alpha = false;
	}

	/* get_desktop_resolution uses shim::adapter, but it's not normally set until shim::start is called, so set it here since
	 * it's used below.
	 */
	int adapter_i = util::check_args(shim::argc, shim::argv, "+adapter");
	if (adapter_i >= 0) {
		shim::adapter = atoi(shim::argv[adapter_i+1]);
		if (shim::adapter >= SDL_GetNumVideoDisplays()-1) {
			shim::adapter = 0;
		}
	}

	gfx::set_minimum_window_size(util::Size<int>(640, 360));
	util::Size<int> desktop_resolution = gfx::get_desktop_resolution();
	gfx::set_maximum_window_size(desktop_resolution);

#if !defined IOS && !defined ANDROID
	const int min_supp_w = 1280;
	const int min_supp_h = 720;

	if (desktop_resolution.w < min_supp_w || desktop_resolution.h < min_supp_h) {
		gui::popup("Unsupported System", "The minimum resolution supported by this game is 1280x720, which this system does not meet. Exiting.", gui::OK);
		exit(1);
	}
#endif

	int win_w = 1280;
	int win_h = 720;

	if (desktop_resolution.w >= 1920*2 && desktop_resolution.h >= 1080*2) {
		win_w = 2560;
		win_h = 1440;
	}
	else if (desktop_resolution.w >= 2560 && desktop_resolution.h >= 1440) {
		win_w = 1920;
		win_h = 1080;
	}

	//if (shim::start_all(0, 0, false, desktop_resolution.w, desktop_resolution.h) == false) {
	if (shim::start_all(640, 360, false, win_w, win_h) == false) {
		gui::fatalerror("ERROR", "Initialization failed", gui::OK, true);
		return false;
	}

#ifdef _WIN32
	gfx::enable_press_and_hold(false);
#endif

	/*
	if (shim::font == 0) {
		gui::fatalerror("Fatal Error", "Font not found! Aborting.", gui::OK, false);
		return false;
	}
	*/

	if (util::bool_arg(false, shim::argc, shim::argv, "dump-images")) {
		std::vector<std::string> filenames = shim::cpa->get_all_filenames();
		for (size_t i = 0; i < filenames.size(); i++) {
			std::string filename =  filenames[i];
			if (filename.find(".tga") != std::string::npos) {
				gfx::Image *image = new gfx::Image(filename, true);
				std::string path = "out/" + filename;
				std::string dir;
				size_t i;
				while ((i = path.find("/")) != std::string::npos) {
					dir += path.substr(0, i);
					util::mkdir(dir.c_str());
					path = path.substr(i+1);
					dir += "/";
				}
				image->save("out/" + filename);
				delete image;
			}
		}
		exit(0);
	}

	TGUI::set_focus_sloppiness(0);

	//gfx::register_lost_device_callbacks(lost_device, found_device);
	//shim::joystick_disconnect_callback = joystick_disconnected;

	return true;
}

void handle_event(TGUI_Event *event)
{
	if (event->type == TGUI_UNKNOWN) {
		return;
	}
	else if (event->type == TGUI_QUIT) {
		quit = true;
	}
}

void draw_all()
{
	gfx::clear(shim::black);

	gfx::set_cull_mode(gfx::NO_FACE);
	
#if defined LUA_BENCH || defined LUA_BENCH2
	call_lua(lua_state, "draw", "");
#elif defined CPP_BENCH2
	SDL_Colour c;
	c.r = 0;
	c.g = 255;
	c.b = 0;
	c.a = 25;
	for (int x = 0; x < 640; x+=2) {
		float y = sin(x/640.0f*M_PI*2)*90;
		gfx::draw_filled_circle(c, util::Point<float>(x, y+180), 8);
	}
	Uint32 now = SDL_GetTicks();
	now /= 16;
	now *= 5;
	now %= 640;
	float y = sin(now/640.0f*M_PI*2)*90;
	c.r = 128;
	c.b = 128;
	gfx::draw_filled_circle(c, util::Point<float>(now, y+180), 32);
#elif defined CPP_BENCH
	int w = 640 / grass->size.w;
	int h = 360 / grass->size.h;
	grass->start_batch();
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			grass->draw(util::Point<float>(x*grass->size.w, y*grass->size.h));
		}
	}
	grass->end_batch();
	for (int i = 0; i < 17; i++) {
		int x = util::rand(0, w-1);
		int y = util::rand(0, h-1);
		robot->draw(util::Point<float>(x*robot->size.w, y*robot->size.h));
	}
#else
	booboo::call_function(prg, "draw", std::vector<std::string>(), "");
#endif

	gfx::draw_guis();
	gfx::draw_notifications();

	gfx::flip();
}

static void loop()
{
	// These keep the logic running at 60Hz and drawing at refresh rate is possible
	// NOTE: screen refresh rate has to be 60Hz or higher for this to work.
	const float target_fps = shim::refresh_rate <= 0 ? 60.0f : shim::refresh_rate;
	Uint32 start = SDL_GetTicks();
	int logic_frames = 0;
	int drawing_frames = 0;
	bool can_draw = true;
	bool can_logic = true;
	std::string old_music_name = "";
#if defined IOS || defined ANDROID
	float old_volume = 1.0f;
#endif
	int curr_logic_rate = shim::logic_rate;

	while (quit == false) {
		// EVENTS
		while (true) {
			SDL_Event sdl_event;
			TGUI_Event *e = nullptr;

			bool all_done = false;

			if (!SDL_PollEvent(&sdl_event)) {
				e = shim::pop_pushed_event();
				if (e == nullptr) {
					all_done = true;
				}
			}

			if (all_done) {
				break;
			}

			if (e == nullptr) {
				if (sdl_event.type == SDL_QUIT) {
					if (can_logic == false) {
						shim::handle_event(&sdl_event);
						quit = true;
						break;
					}
				}
				else if (sdl_event.type == SDL_KEYDOWN && sdl_event.key.keysym.sym == SDLK_F12) {
					//load_translation();
				}
			}

			TGUI_Event *event;

			if (e) {
				event = e;
			}
			else {
				if (sdl_event.type == SDL_QUIT) {
					static TGUI_Event quit_event;
					quit_event.type = TGUI_QUIT;
					event = &quit_event;
				}
				else {
					event = shim::handle_event(&sdl_event);
				}
			}

			handle_event(event);
		}

		// Logic rate can change in devsettings
		if (shim::logic_rate != curr_logic_rate) {
			curr_logic_rate = shim::logic_rate;
			logic_frames = 0;
			drawing_frames = 0;
			start = SDL_GetTicks();
		}

		// TIMING
		Uint32 now = SDL_GetTicks();
		int diff = now - start;
		bool skip_drawing = false;
		int logic_reps = diff / 16;

		if (logic_reps > 0) {
			start += 16 * logic_reps;
		}

		for (int logic = 0; logic < logic_reps; logic++) {
			can_draw = shim::update();

			// Generate a timer tick event (TGUI_TICK)
			SDL_Event sdl_event;
			sdl_event.type = shim::timer_event_id;
			TGUI_Event *event = shim::handle_event(&sdl_event);
			handle_event(event);

#if defined LUA_BENCH2
			call_lua(lua_state, "run", "");
#else
			booboo::call_function(prg, "run", std::vector<std::string>(), "");
#endif

			if (booboo::reset_game_name != "") {
				quit = true;
			}

			logic_frames++;
		}

		// LOGIC
		if (can_logic) {
			for (int logic = 0; logic < logic_reps; logic++) {
				gfx::update_animations();
				// logic
			}
		}

		// DRAWING
		if (skip_drawing == false && can_draw) {
			draw_all();
		}

		if (quit) {
			break;
		}

		drawing_frames++;
	}
}

bool go()
{
	loop();

	return true;
}

void end()
{
	// If Alt-F4 is pressed the title gui can remain in shim::guis. Leaving it to shim to destruct won't work, because ~Title_GUI accesses Globals which is destroyed below
	for (std::vector<gui::GUI *>::iterator it = shim::guis.begin(); it != shim::guis.end();) {
		gui::GUI *gui = *it;
		delete gui;
		it = shim::guis.erase(it);
	}

	shim::end_all();
}

static int set_orig_args(bool forced, bool count_only)
{
	int count = 0;

	for (int i = 0; i < orig_argc; i++) {
		int skip = 0;
		if (forced &&
			(std::string(orig_argv[i]) == "+windowed" ||
			std::string(orig_argv[i]) == "+fullscreen")) {
			skip = 1;
		}
		else if (forced &&
			(std::string(orig_argv[i]) == "+width" ||
			std::string(orig_argv[i]) == "+height" ||
			std::string(orig_argv[i]) == "+scale")) {
			skip = 2;
		}

		if (skip > 0) {
			i += skip-1;
			continue;
		}

		if (count_only == false) {
			shim::argv[count] = new char[strlen(orig_argv[i])+1];
			strcpy(shim::argv[count], orig_argv[i]);
		}
		
		count++;
	}

	return count;
}

void set_shim_args(bool initial, bool force_windowed, bool force_fullscreen)
{
	if (initial) {
		for (int i = 0; i < orig_argc; i++) {
			if (std::string(orig_argv[i]) == "+windowed" || std::string(orig_argv[i]) == "+fullscreen" || std::string(orig_argv[i]) == "+width" || std::string(orig_argv[i]) == "+height" || std::string(orig_argv[i]) == "+adapter") {
				force_windowed = false;
				force_fullscreen = false;
				break;
			}
		}
	}

	bool force = force_windowed || force_fullscreen;
	
	int count = set_orig_args(force, true);

	if (force) {
		count++;
	}

	std::vector<std::string> v;
	if (extra_args != "") {
		util::Tokenizer t(extra_args, ',');
		std::string tok;
		while ((tok = t.next()) != "") {
			v.push_back(tok);
		}
		count += v.size();
	}
	extra_args = ""; // Do this?

	shim::argc = count;
	shim::argv = new char *[count];

	int i = set_orig_args(force, false);

	if (force_windowed) {
		shim::argv[i] = new char[strlen("+windowed")+1];
		strcpy(shim::argv[i], "+windowed");
		i++;
	}
	else if (force_fullscreen) {
		shim::argv[i] = new char[strlen("+fullscreen")+1];
		strcpy(shim::argv[i], "+fullscreen");
		i++;
	}

	for (auto s : v) {
		shim::argv[i] = new char[s.length()+1];
		strcpy(shim::argv[i], s.c_str());
		i++;
	}
}

int main(int argc, char **argv)
{
	try {

#ifdef _WIN32
	SDL_RegisterApp("BooBoo", 0, 0);
#endif

	orig_argc = argc;
	orig_argv = argv;

	// this must be before static_start which inits SDL
#ifdef _WIN32
	bool directsound = util::bool_arg(true, argc, argv, "directsound");
	bool wasapi = util::bool_arg(false, argc, argv, "wasapi");
	bool winmm = util::bool_arg(false, argc, argv, "winmm");

	if (directsound) {
		_putenv_s("SDL_AUDIODRIVER", "directsound");
	}
	else if (wasapi) {
		_putenv_s("SDL_AUDIODRIVER", "wasapi");
	}
	else if (winmm) {
		_putenv_s("SDL_AUDIODRIVER", "winmm");
	}
#endif

	shim::window_title = "BooBoo";
	shim::organisation_name = "b1stable";
	shim::game_name = "BooBoo";
	//
	shim::logging = true;
	gfx::Image::ignore_palette = true;

	// Need to be set temporarily to original values so +error-level works. They get changed below
	shim::argc = orig_argc;
	shim::argv = orig_argv;

	//set_shim_args(true, false, true);
	set_shim_args(true, true, false);

	shim::static_start_all();

	util::srand((uint32_t)time(NULL));

	//shim::create_depth_buffer = true;

	start();

	std::string fn;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '+' && argv[i][0] != '-') {
			fn = argv[i];
			break;
		}
	}

again:
	quit = false;

	if (booboo::reset_game_name != "") {
		if (booboo::load_from_filesystem) {
			fn = booboo::reset_game_name;
		}
		else {
			fn = std::string("code/") + booboo::reset_game_name;
		}
		booboo::reset_game_name = "";
	}

	booboo::start();

	std::string code;

	if (load_from_filesystem_set) {
		if (booboo::load_from_filesystem) {
			try {
				code = util::load_text_from_filesystem(fn);
			}
			catch (util::Error &e) {
				gui::fatalerror("ERROR", "Program is missing or corrupt!", gui::OK, true);
			}
		}
		else {
			try {
				code = util::load_text(fn);
			}
			catch (util::Error &e) {
				gui::fatalerror("ERROR", "Program is missing or corrupt!", gui::OK, true);
			}
		}
	}
	else {
		if (fn != "") {
			try {
				code = util::load_text_from_filesystem(fn);
				booboo::load_from_filesystem = true;
			}
			catch (util::Error &e) {
				gui::fatalerror("ERROR", "Program is missing or corrupt!", gui::OK, true);
			}
		}
		else {
			try {
				code = util::load_text_from_filesystem("main.boo");
				booboo::load_from_filesystem = true;
			}
			catch (util::Error &e) {
				try {
					code = util::load_text("code/main.boo");
					booboo::load_from_filesystem = false;
				}
				catch (util::Error &e) {
					gui::fatalerror("ERROR", "Program is missing or corrupt!", gui::OK, true);
				}
			}
		}
	}


	load_from_filesystem_set = true;
	
	prg = booboo::create_program(code);
	
	// This does one token at a time, and we want it to finish the main body so it's a loop
	while (booboo::interpret(prg)) {
	}

#if defined LUA_BENCH || defined LUA_BENCH2
	init_lua();
#elif defined CPP_BENCH
	grass = new gfx::Image("misc/grass.tga");
	robot = new gfx::Image("misc/robot.tga");
#endif

#ifdef DUMP
	printf("main:\n");
	for (size_t i = 0; i < prg.program.size(); i++) {
		booboo::Statement &s = prg.program[i];
		printf("%d ", s.method);
		for (size_t j = 0; j < s.data.size(); j++) {
			printf("%s ", s.data[j].token.c_str());
		}
		printf("\n");
	}
	std::map<std::string, booboo::Label>::iterator it2;
	for (it2 = prg.labels.begin(); it2 != prg.labels.end(); it2++) {
		std::string label = (*it2).first;
		printf("label %s %d\n", label.c_str(), (*it2).second.pc);
	}
	std::map<std::string, booboo::Program>::iterator it;
	for (it = prg.functions.begin(); it != prg.functions.end(); it++) {
		booboo::Program prg = (*it).second;
		printf("%s:\n", (*it).first.c_str());
		for (size_t i = 0; i < prg.program.size(); i++) {
			booboo::Statement &s = prg.program[i];
			printf("%d ", s.method);
			for (size_t j = 0; j < s.data.size(); j++) {
				printf("%s ", s.data[j].token.c_str());
			}
			printf("\n");
		}
		std::map<std::string, booboo::Label>::iterator it2;
		for (it2 = prg.labels.begin(); it2 != prg.labels.end(); it2++) {
			std::string label = (*it2).first;
			printf("label %s %d\n", label.c_str(), (*it2).second.pc);
		}
	}
#endif
	
	if (booboo::reset_game_name == "") {
		go();
	}

	booboo::call_function(prg, "end", std::vector<std::string>(), "");

	booboo::destroy_program(prg);

	if (booboo::reset_game_name != "") {
		fn = "";
		goto again;
	}

	end();

	booboo::end();

	}
	catch (util::Error &e) {
		gui::fatalerror("ERROR", e.error_message.c_str(), gui::OK, true);
		printf("%s\n", e.error_message.c_str());
	}

	return booboo::return_code;
}
