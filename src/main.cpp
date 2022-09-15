#include <shim4/shim4.h>

#include "beepboop.h"

bool load_from_filesystem;
bool load_from_filesystem_set;
std::string reset_game_name;

static bool quit;

PROGRAM prg;

int orig_argc;
char **orig_argv;

std::string extra_args;
std::string extra_args_orig;

static void found_device()
{
}

static void lost_device()
{
}

static void joystick_disconnected()
{
}

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

	gfx::register_lost_device_callbacks(lost_device, found_device);
	shim::joystick_disconnect_callback = joystick_disconnected;

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

	PROGRAM p;
	bool found = false;

	for (size_t i = 0; i < prg.functions.size(); i++) {
		if (prg.functions[i].name == "draw") {
			p = prg.functions[i];
			found = true;
			break;
		}
	}

	if (found) {
		p.variables = prg.variables;
		p.functions = prg.functions;
		p.mml_id = prg.mml_id;
		p.image_id = prg.image_id;
		p.font_id = prg.font_id;
		p.vector_id = prg.vector_id;
		p.mmls = prg.mmls;
		p.images = prg.images;
		p.fonts = prg.fonts;
		p.vectors = prg.vectors;
		
		process_includes(p);
		p.labels = process_labels(p);

		try {
			while (interpret(p)) {
			}
		}
		catch (EXCEPTION e) {
			gui::fatalerror("ERROR", e.error.c_str(), gui::OK, true);
		}
				
		for (size_t i = 0; i < p.variables.size(); i++) {
			if (p.variables[i].function == "main") {
				for (size_t j = 0; j < prg.variables.size(); j++) {
					if (p.variables[i].name == prg.variables[j].name) {
						prg.variables[j] = p.variables[i];
					}
				}
			}
		}

		for (std::map< int, std::vector<VARIABLE> >::iterator it = prg.vectors.begin(); it != prg.vectors.end(); it++) {
			std::pair< int, std::vector<VARIABLE> > pair = *it;
			prg.vectors[pair.first] = p.vectors[pair.first];
		}
	}

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

			PROGRAM p;
			bool found = false;

			for (size_t i = 0; i < prg.functions.size(); i++) {
				if (prg.functions[i].name == "logic") {
					p = prg.functions[i];
					found = true;
					break;
				}
			}

			if (found) {
				p.variables = prg.variables;
				p.functions = prg.functions;
				p.mml_id = prg.mml_id;
				p.image_id = prg.image_id;
				p.font_id = prg.font_id;
				p.vector_id = prg.vector_id;
				p.mmls = prg.mmls;
				p.images = prg.images;
				p.fonts = prg.fonts;
				p.vectors = prg.vectors;

				process_includes(p);
				p.labels = process_labels(p);

				try {
					while (interpret(p)) {
					}
				}
				catch (EXCEPTION e) {
					gui::fatalerror("ERROR", e.error.c_str(), gui::OK, true);
				}
				
				for (size_t i = 0; i < p.variables.size(); i++) {
					if (p.variables[i].function == "main") {
						for (size_t j = 0; j < prg.variables.size(); j++) {
							if (p.variables[i].name == prg.variables[j].name) {
								prg.variables[j] = p.variables[i];
							}
						}
					}
					
					for (std::map< int, std::vector<VARIABLE> >::iterator it = prg.vectors.begin(); it != prg.vectors.end(); it++) {
						std::pair< int, std::vector<VARIABLE> > pair = *it;
						prg.vectors[pair.first] = p.vectors[pair.first];
					}
				}
			}

			if (reset_game_name != "") {
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
#ifdef _WIN32
	SDL_RegisterApp("BeepBoop", 0, 0);
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

	shim::window_title = "BeepBoop";
	shim::organisation_name = "b1stable";
	shim::game_name = "BeepBoop";
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

	std::string fn = argc > 1 ? argv[1] : "";

again:
	try {

	quit = false;

	if (reset_game_name != "") {
		if (load_from_filesystem) {
			fn = reset_game_name;
		}
		else {
			fn = std::string("code/") + reset_game_name;
		}
		reset_game_name = "";
	}

	prg.name = "main";
	prg.mml_id = 0;
	prg.image_id = 0;
	prg.font_id = 0;
	prg.vector_id = 0;
	if (load_from_filesystem_set) {
		if (load_from_filesystem) {
			try {
				prg.code = util::load_text_from_filesystem(fn);
			}
			catch (util::Error e) {
				gui::fatalerror("ERROR", "Program is missing or corrupt!", gui::OK, true);
			}
		}
		else {
			try {
				prg.code = util::load_text(fn);
			}
			catch (util::Error e) {
				gui::fatalerror("ERROR", "Program is missing or corrupt!", gui::OK, true);
			}
		}
	}
	else {
		if (fn != "") {
			try {
				prg.code = util::load_text_from_filesystem(fn);
				load_from_filesystem = true;
			}
			catch (util::Error e) {
				gui::fatalerror("ERROR", "Program is missing or corrupt!", gui::OK, true);
			}
		}
		else {
			try {
				prg.code = util::load_text_from_filesystem("main.bb");
				load_from_filesystem = true;
			}
			catch (util::Error e) {
				try {
					prg.code = util::load_text("code/main.bb");
					load_from_filesystem = false;
				}
				catch (util::Error e) {
					gui::fatalerror("ERROR", "Program is missing or corrupt!", gui::OK, true);
				}
			}
		}
	}
	load_from_filesystem_set = true;
	prg.line = 1;
	prg.start_line = 0;
	prg.p = 0;
	
	process_includes(prg);

	try {
		prg.labels = process_labels(prg);

		while (interpret(prg)) {
		}
	}
	catch (EXCEPTION e) {
		gui::fatalerror("ERROR", e.error.c_str(), gui::OK, true);
	}
	
	PROGRAM p;
	bool found = false;

	for (size_t i = 0; i < prg.functions.size(); i++) {
		if (prg.functions[i].name == "init") {
			p = prg.functions[i];
			found = true;
			break;
		}
	}

	if (found) {
		p.variables = prg.variables;
		p.functions = prg.functions;
		p.mml_id = prg.mml_id;
		p.image_id = prg.image_id;
		p.font_id = prg.font_id;
		p.vector_id = prg.vector_id;
		p.mmls = prg.mmls;
		p.images = prg.images;
		p.fonts = prg.fonts;
		p.vectors = prg.vectors;
		
		p.labels = process_labels(p);
		process_includes(p);

		try {
			while (interpret(p)) {
			}
		}
		catch (EXCEPTION e) {
			gui::fatalerror("ERROR", e.error.c_str(), gui::OK, true);
		}
				
		for (size_t i = 0; i < p.variables.size(); i++) {
			if (p.variables[i].function == "main") {
				for (size_t j = 0; j < prg.variables.size(); j++) {
					if (p.variables[i].name == prg.variables[j].name) {
						prg.variables[j] = p.variables[i];
					}
				}
			}
		}
		
		for (std::map< int, std::vector<VARIABLE> >::iterator it = prg.vectors.begin(); it != prg.vectors.end(); it++) {
			std::pair< int, std::vector<VARIABLE> > pair = *it;
			prg.vectors[pair.first] = p.vectors[pair.first];
		}
	}

	if (reset_game_name == "") {
		go();
	}

	found = false;

	for (size_t i = 0; i < prg.functions.size(); i++) {
		if (prg.functions[i].name == "shutdown") {
			p = prg.functions[i];
			found = true;
			break;
		}
	}

	if (found) {
		p.variables = prg.variables;
		p.functions = prg.functions;
		p.mml_id = prg.mml_id;
		p.image_id = prg.image_id;
		p.font_id = prg.font_id;
		p.vector_id = prg.vector_id;
		p.mmls = prg.mmls;
		p.images = prg.images;
		p.fonts = prg.fonts;
		p.vectors = prg.vectors;

		p.labels = process_labels(p);
		process_includes(p);

		try {
			while (interpret(p)) {
			}
		}
		catch (EXCEPTION e) {
			gui::fatalerror("ERROR", e.error.c_str(), gui::OK, true);
		}
				
		for (size_t i = 0; i < p.variables.size(); i++) {
			if (p.variables[i].function == "main") {
				for (size_t j = 0; j < prg.variables.size(); j++) {
					if (p.variables[i].name == prg.variables[j].name) {
						prg.variables[j] = p.variables[i];
					}
				}
			}
		}
		
		for (std::map< int, std::vector<VARIABLE> >::iterator it = prg.vectors.begin(); it != prg.vectors.end(); it++) {
			std::pair< int, std::vector<VARIABLE> > pair = *it;
			prg.vectors[pair.first] = p.vectors[pair.first];
		}
	}

	destroy_program(prg, true);

	}
	catch (util::Error e) {
		gui::fatalerror("ERROR", e.error_message.c_str(), gui::OK, true);
	}

	if (reset_game_name != "") {
		fn = "";
		goto again;
	}

	end();

	return 0;
}
