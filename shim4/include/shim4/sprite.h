#ifndef NOO_SPRITE_H
#define NOO_SPRITE_H

#include "shim4/main.h"

namespace noo {

namespace gfx {

class Image;

class Sprite {
public:
	struct Animation {
		Image *parent;
		std::vector<Image *> images;
		std::vector<Uint32> delays;
		Uint32 total_delays;
		bool rand_start;
		bool looping;
		util::Point<int> opaque_topleft;
		util::Point<int> opaque_bottomright;
	};

	struct Instance {
		bool started;
		Uint32 elapsed;
		// "" when not set
		std::string current_animation;
		util::Callback finished_callback;
		void *finished_callback_data;
		std::map<std::string, Animation *> animations;
	};

	struct Loaded_Sprite {
		std::vector<Instance *> instances;
	};

	static void static_start();
	static void update_all(); // called each logic tick from shim::update
	
	static std::map<std::string, Loaded_Sprite *> loaded_sprites;

	SHIM4_EXPORT Sprite(std::string json_filename, std::string image_directory, bool absolute_path = false);
	SHIM4_EXPORT Sprite(std::string image_directory);
	SHIM4_EXPORT ~Sprite();

	SHIM4_EXPORT bool set_animation(std::string name, util::Callback finished_callback = 0, void *finished_callback_data = 0);
	SHIM4_EXPORT std::string get_animation();
	SHIM4_EXPORT std::string get_previous_animation();

	SHIM4_EXPORT void start();
	SHIM4_EXPORT void stop();
	SHIM4_EXPORT void reset(); // set to frame 0
	SHIM4_EXPORT bool is_started();
	SHIM4_EXPORT bool is_finished();

	SHIM4_EXPORT int get_current_frame();
	SHIM4_EXPORT int get_num_frames();
	SHIM4_EXPORT std::vector<Uint32> get_frame_times();
	
	SHIM4_EXPORT int get_length();

	SHIM4_EXPORT Image *get_current_image();
	SHIM4_EXPORT Image *get_image(int frame);

	SHIM4_EXPORT void set_reverse(bool reverse); // play from back to front
	SHIM4_EXPORT bool is_reversed();

	SHIM4_EXPORT void get_filenames(std::string &json_filename, std::string &image_directory);

	SHIM4_EXPORT void sync_with(Sprite *sprite, bool match_animation = false); // match timing with input sprite

	SHIM4_EXPORT void get_bounds(util::Point<int> &topleft, util::Point<int> &bottomright);

	SHIM4_EXPORT Uint32 get_elapsed();

	SHIM4_EXPORT void update();

	SHIM4_EXPORT void set_rand_start(bool rand_start);

	SHIM4_EXPORT Animation *get_animation(std::string name);

	SHIM4_EXPORT gfx::Image *get_parent_image(); // of current animation

private:
	static void update_loaded_sprite(std::string dirname, Sprite::Loaded_Sprite *s);

	SHIM4_EXPORT void load(std::string json_filename, std::string image_directory, bool absolute_path = false);
	Instance *instance;
	std::string previous_animation;
	bool reverse;

	std::string json_filename;
	std::string image_directory;
};

} // End namespace gfx

} // End namespace noo

#endif // NOO_SPRITE_H
