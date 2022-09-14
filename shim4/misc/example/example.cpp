// An example game loop. Fill out the following functions and data to make a game.

#include <Nooskewl_Shim/Nooskewl_Shim.h>

using namespace noo;

// Fill out this data
static std::string window_title = "Example";
static std::string organization_name = "Nooskewl";
static std::string game_name = "Example";

// Fill out these functions

static bool my_start()
{
	// put your initialization code here. return false to signify failure
	return true;
}

static void my_end()
{
	// clean up your stuff here
}

static void my_handle_event(TGUI_Event *event)
{
	// handle input events
}

static void my_logic()
{
	// do your game logic here. this gets called 60 times per second
}

static void my_drawing()
{
	// draw your graphics here
}

// ---------------------------------------------------------------------------

// The rest does all the nitty gritty stuff

static bool start(int argc, char **argv)
{
#ifdef _WIN32
	std::string without_special_characters;
	for (size_t i = 0; i < game_name.length(); i++) {
		char c = game_name[i];
		if (isalpha(c)) {
			char s[2] = { c, 0 };
			without_special_characters += s;
		}
	}
	SDL_RegisterApp((char *)without_special_characters.c_str(), 0, 0);
#endif

	shim::window_title = window_title;
	shim::organization_name = organization_name;
	shim::game_name = game_name;

	if (shim::start(argc, argv) == false) {
		return false;
	}

	return true;
}

static void loop()
{
	bool quit = false;

	/* These keep the logic running at 60Hz and drawing at refresh rate if
	 * possible. vsync can be disabled and logic will still run at the same
	 * speed, even if refresh rate is higher than 60Hz.
	 */
	const float target_logic_rate = 60.0f;
	const float target_fps = (float)shim::refresh_rate;
	Uint32 start = SDL_GetTicks();
	int logic_frames = 0;
	int drawing_frames = 0;

	while (quit == false) {
		// EVENTS
		SDL_Event sdl_event;

		while (SDL_PollEvent(&sdl_event)) {
			if (sdl_event.type == SDL_QUIT) {
				quit = true;
				break;
			}
			TGUI_Event *e = shim::handle_event(&sdl_event);

			my_handle_event(e);
		}

		if (quit) {
			break;
		}

		// TIMING
		int diff = SDL_GetTicks() - start;
		bool skip_logic;
		bool skip_drawing;

		if (diff > 0) {
			float average;
			// Skip logic if running fast
			average = logic_frames / (diff / 1000.0f);
			if (average > (target_logic_rate+0.1f)) { // a little leeway
				skip_logic = true;
			}
			else {
				skip_logic = false;
			}
			// Skip drawing if running fast
			average = drawing_frames / (diff / 1000.0f);
			if (average < (target_fps-2.0f)) { // allow a little bit of fluctuation, i.e., not exactly target_fps here
				skip_drawing = true;
			}
			else {
				skip_drawing = false;
			}
		}
		else {
			skip_logic = skip_drawing = false;
		}

		// LOGIC
		if (skip_logic == false) {
			if (shim::update() == false) {
				break;
			}
			my_logic();
			logic_frames++;
		}

		// DRAWING
		if (skip_drawing == false) {
			gfx::start_draw();
			my_drawing();
			gfx::end_draw();
			gfx::flip();
		}

		drawing_frames++;
	}
}

int main(int argc, char **argv)
{
	try {
		if (start(argc, argv) == false || my_start() == false) {
			util::errormsg("Error during initialization\n");
			return 1;
		}

		loop();

		my_end();

		shim::end();
	}
	catch (util::Error e) {
		util::errormsg("Fatal error: %s\n", e.error_message.c_str());
		return 1;
	}

	return 0;
}
