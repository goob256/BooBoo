#include "shim3/shim3.h"

using namespace noo;

int main(int argc, char **argv)
{
#ifdef _WIN32
	_putenv_s("SDL_AUDIODRIVER", "directsound");
#endif

	if (shim::static_start_all(SDL_INIT_AUDIO) == false) {
		return 1;
	}

	shim::argc = argc;
	shim::argv = argv;
	shim::use_cwd = true;
	shim::error_level = 3;
	shim::log_tags = false;

	if (util::start() == false || audio::start() == false) {
		return 1;
	}

	if (argc < 2) {
		util::infomsg("Usage: %s <filename.mml>\n", argv[0]);
		return 0;
	}

	bool loop = util::check_args(argc, argv, "+loop") >= 0;

	audio::MML *mml = new audio::MML(argv[1], true);

	mml->play(1.0f, loop);

	while (!mml->is_done()) {
		SDL_Delay(1);
	}

	SDL_Delay(1000); // let any buffered audio finish

	delete mml;

	audio::end();

	util::end();

	shim::static_end();

	return 0;
}
