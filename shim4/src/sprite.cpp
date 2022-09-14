#include "shim4/error.h"
#include "shim4/image.h"
#include "shim4/json.h"
#include "shim4/mt.h"
#include "shim4/shim.h"
#include "shim4/sprite.h"
#include "shim4/tokenizer.h"
#include "shim4/util.h"

using namespace noo;

namespace noo {

namespace gfx {

std::map<std::string, Sprite::Loaded_Sprite *> Sprite::loaded_sprites;

void Sprite::static_start()
{
	loaded_sprites.clear();
}

void Sprite::update_loaded_sprite(std::string dirname, Sprite::Loaded_Sprite *s)
{
	Uint32 elapsed = 1000 / shim::logic_rate;

	size_t i = 0;

loop:

	for (; i < s->instances.size(); i++) {
		Sprite::Instance *instance = s->instances[i];
		if (instance->started) {
			instance->elapsed += elapsed;
			Sprite::Animation *anim = instance->animations[instance->current_animation];
			if (instance->finished_callback != 0 && instance->elapsed >= anim->total_delays) {
				instance->elapsed = instance->elapsed % anim->total_delays;
				// Back up so you can chain these
				util::Callback bak_callback = instance->finished_callback;
				void *bak_data = instance->finished_callback_data;
				instance->finished_callback = 0;
				// Don't need to unset data, though it could change
				bak_callback(bak_data);
				// the callback could delete the sprite which might remove it from s->instances or remove the Loaded_Sprite from loaded_sprites... we need to check
				if (loaded_sprites.find(dirname) == loaded_sprites.end()) {
					return;
				}
				else {
					if (std::find(s->instances.begin(), s->instances.end(), instance) == s->instances.end()) {
						goto loop;
					}
				}
			}
		}
	}
}

void Sprite::update_all()
{
	std::map<std::string, Loaded_Sprite *> tmp = loaded_sprites;

	std::map<std::string, Loaded_Sprite *>::iterator it;

	for (it = tmp.begin(); it != tmp.end(); it++) {
		const std::pair<std::string, Loaded_Sprite *> &p = *it;
		update_loaded_sprite(p.first, p.second);
	}
}

Sprite::Sprite(std::string json_filename, std::string image_directory, bool absolute_path)
{
	instance = new Instance;
	instance->started = false;
	instance->finished_callback = 0;
	instance->elapsed = 0;

	reverse = false;

	load(json_filename, image_directory, absolute_path);
	start();
}

Sprite::Sprite(std::string image_directory)
{
	instance = new Instance;
	instance->started = false;
	instance->finished_callback = 0;
	instance->elapsed = 0;

	reverse = false;

	load(image_directory + "/sprite.json", image_directory);
	start();
}

Sprite::~Sprite()
{
	std::map<std::string, Animation *>::iterator it;
	for (it = instance->animations.begin(); it != instance->animations.end(); it++) {
		std::pair<std::string, Animation *> p = *it;
		Animation *a = p.second;
		for (size_t i = 0; i < a->images.size(); i++) {
			delete a->images[i];
		}
		delete a->parent;
		delete a;
	}

	std::map<std::string, Loaded_Sprite *>::iterator it2 = loaded_sprites.find(this->image_directory);
	if (it2 != loaded_sprites.end()) {
		const std::pair<std::string, Loaded_Sprite *> &p = *it2;
		Loaded_Sprite *s = p.second;
		std::vector<Instance *>::iterator it3 = std::find(s->instances.begin(), s->instances.end(), instance);
		if (it3 != s->instances.end()) {
			Instance *i = *it3;
			delete i;
			s->instances.erase(it3);
		}
		if (s->instances.size() == 0) {
			delete s;
			loaded_sprites.erase(it2);
		}
	}
}

void Sprite::load(std::string json_filename, std::string image_directory, bool absolute_path)
{
	if (absolute_path == false) {
		json_filename = "gfx/sprites/" + json_filename;
		image_directory = "gfx/sprites/" + image_directory;
	}

	this->json_filename = json_filename;
	this->image_directory = image_directory;

	util::JSON *json;
	try {
		json = new util::JSON(image_directory + "/sprite.json");
	}
	catch (util::Error &) {
		delete instance;
		throw;
	}
	util::JSON::Node *root = json->get_root();

	bool first = true;

	for (size_t anim_num = 0; anim_num < root->children.size(); anim_num++) {
		util::JSON::Node *anim = root->children[anim_num];
		util::JSON *json = nullptr;
		std::vector<Image *> images;
		std::vector<Uint32> delays_vector;
		Image *parent;
		bool had_keep_data;
	       	try {
			json = new util::JSON(image_directory + "/" + anim->value + ".json");
			util::JSON::Node *frames = json->get_root()->find("frames");
			if (frames == 0) {
				throw util::Error("Invalid sprite JSON::Node, no frames hash");
			}
			int count = (int)frames->children.size();
			had_keep_data = Image::keep_data;
			Image::keep_data = true;
			parent = new Image(image_directory + "/" + anim->value + ".tga",  true);
			for (int i = 0; i < count; i++) {
				util::JSON::Node *json = frames->children[i];
				util::JSON::Node *frame = json->find("frame");
				if (frame == 0) {
					throw util::Error("JSON::Node parsing error, frame missing frame hash");
				}
				util::JSON::Node *jx = frame->find("x");
				if (jx == 0) {
					throw util::Error("JSON::Node parsing error, frame missing x");
				}
				util::JSON::Node *jy = frame->find("y");
				if (jy == 0) {
					throw util::Error("JSON::Node parsing error, frame missing y");
				}
				util::JSON::Node *jw = frame->find("w");
				if (jw == 0) {
					throw util::Error("JSON::Node parsing error, frame missing w");
				}
				util::JSON::Node *jh = frame->find("h");
				if (jh == 0) {
					throw util::Error("JSON::Node parsing error, frame missing h");
				}
				util::JSON::Node *jduration = json->find("duration");
				if (jduration == 0) {
					throw util::Error("JSON::Node parsing error, frame missing duration");
				}
				int x = atoi(jx->value.c_str());
				int y = atoi(jy->value.c_str());
				int w = atoi(jw->value.c_str());
				int h = atoi(jh->value.c_str());
				int duration = atoi(jduration->value.c_str());
				Image *sub = new Image(parent, {x, y}, {w, h});
				images.push_back(sub);
				delays_vector.push_back(duration);
			}
		}
		catch (util::Error &) {
			std::string s = util::load_text(image_directory + "/" + anim->value + ".csv");
			util::Tokenizer t(s, '\n');
			t.next();
			parent = new Image(image_directory + "/" + anim->value + ".tga",  true);
			int w_orig = 0;
			int h_orig = 0;
			while (true) {
				std::string line = t.next();
				util::trim(line);
				if (line == "") {
					break;
				}
				util::Tokenizer t2(line, ',');
				std::string delay = util::JSON::trim_quotes(t2.next());
				std::string x = util::JSON::trim_quotes(t2.next());
				std::string y = util::JSON::trim_quotes(t2.next());
				std::string w = util::JSON::trim_quotes(t2.next());
				std::string h = util::JSON::trim_quotes(t2.next());
				if (delay == "" || x == "" || y == "" || w == "" || h == "") {
					break;
				}
				int delayi = atoi(delay.c_str());
				int xi = atoi(x.c_str());
				int yi = atoi(y.c_str());
				int wi = atoi(w.c_str());
				int hi = atoi(h.c_str());
				if (images.size() == 0) {
					w_orig = wi;
					h_orig = hi;
				}
				else {
					wi = w_orig;
					hi = h_orig;
				}
				Image *sub = new Image(parent, {xi, yi}, {wi, hi});
				images.push_back(sub);
				delays_vector.push_back(delayi);
			}
		}
		bool looping = true; // FIXME load these from json/aseprite tags
		Uint32 total_delays = 0;
		for (size_t i = 0; i < delays_vector.size(); i++) {
			total_delays += delays_vector[i];
		}
		Animation *a;
		if (instance->animations.find(anim->value) == instance->animations.end()) {
			a = new Animation();
			a->parent = parent;
			a->images = images;
			a->delays = delays_vector;
			a->total_delays = total_delays;
			//a->rand_start = anim->find("rand_start") != 0; FIXME
			a->rand_start = false;
			a->looping = looping;
			a->opaque_topleft = {-1, -1};
			a->opaque_bottomright = {-1, -1};
			for (size_t i = 0; i < images.size(); i++) {
				util::Point<int> this_topleft;
				util::Point<int> this_bottomright;
				images[i]->get_bounds(this_topleft, this_bottomright);
				if (a->opaque_topleft.x == -1 || this_topleft.x < a->opaque_topleft.x) {
					a->opaque_topleft.x = this_topleft.x;
				}
				if (a->opaque_topleft.y == -1 || this_topleft.y < a->opaque_topleft.y) {
					a->opaque_topleft.y = this_topleft.y;
				}
				if (a->opaque_bottomright.x == -1 || this_bottomright.x > a->opaque_bottomright.x) {
					a->opaque_bottomright.x = this_bottomright.x;
				}
				if (a->opaque_bottomright.y == -1 || this_bottomright.y > a->opaque_bottomright.y) {
					a->opaque_bottomright.y = this_bottomright.y;
				}
			}
			instance->animations[anim->value] = a;
		}
		else {
			throw util::Error("Duplicate animation!");
		}
		if (first) {
			first = false;
			instance->current_animation = anim->value;
		}
		delete json;
		Image::keep_data = had_keep_data;
		// Don't destroy parent data here! Needed by sub images in future instances to get bounds
	}

	std::map<std::string, Loaded_Sprite *>::iterator it = loaded_sprites.find(this->image_directory);
	if (it != loaded_sprites.end()) {
		const std::pair<std::string, Loaded_Sprite *> &p = *it;
		Loaded_Sprite *s = p.second;
		s->instances.push_back(instance);
	}
	else {
		Loaded_Sprite *s = new Loaded_Sprite;
		s->instances.push_back(instance);
		loaded_sprites[this->image_directory] = s;
	}

	delete json;
}

bool Sprite::set_animation(std::string name, util::Callback finished_callback, void *finished_callback_data)
{
	// Make sure callbacks get called when animation changes
	if (instance->finished_callback) {
		util::Callback bak_callback = instance->finished_callback;
		void *bak_data = instance->finished_callback_data;
		instance->finished_callback = 0;
		bak_callback(bak_data);
	}

	// Always update these?
	instance->finished_callback = finished_callback;
	instance->finished_callback_data = finished_callback_data;

	if (instance->animations.find(name) == instance->animations.end()) {
		return false;
	}

	if (instance->current_animation != name) {
		previous_animation = instance->current_animation;
		instance->current_animation = name;
	}

	reset();
	start();

	Animation *anim = instance->animations[instance->current_animation];
	if (anim->rand_start) {
		instance->elapsed = util::rand() % anim->total_delays;
	}

	return true;
}

std::string Sprite::get_animation()
{
	return instance->current_animation;
}

std::string Sprite::get_previous_animation()
{
	return previous_animation;
}

void Sprite::start()
{
	if (instance->started == true) {
		return;
	}
	instance->started = true;
}

void Sprite::stop()
{
	if (instance->started == false) {
		return;
	}
	instance->started = false;
}

void Sprite::reset()
{
	instance->elapsed = 0;
}

bool Sprite::is_started()
{
	return instance->started;
}

int Sprite::get_length()
{
	return instance->animations[instance->current_animation]->total_delays;
}

void Sprite::set_reverse(bool reverse)
{
	this->reverse = reverse;
}

bool Sprite::is_reversed()
{
	return reverse;
}


int Sprite::get_current_frame()
{
	Animation *anim = instance->animations[instance->current_animation];

	if (anim->total_delays == 0) {
		return 0;
	}

	Uint32 remainder = instance->elapsed % anim->total_delays;
	int frame;

	if (reverse) {
		frame = (int)anim->delays.size()-1;
		for (int i = (int)anim->delays.size()-1; i >= 0; i--) {
			Uint32 delay = anim->delays[i];
			if (remainder >= delay) {
				remainder -= delay;
				frame--;
			}
			else {
				break;
			}
		}
	}
	else {
		frame = 0;
		for (size_t i = 0; i < anim->delays.size(); i++) {
			Uint32 delay = anim->delays[i];
			if (remainder >= delay) {
				remainder -= delay;
				frame++;
			}
			else {
				break;
			}
		}
	}

	return frame;
}

Image *Sprite::get_current_image()
{
	Animation *anim = instance->animations[instance->current_animation];

	// Don't loop if loop flag off
	if (anim->looping == false && instance->elapsed >= anim->total_delays) {
		return anim->images[anim->images.size()-1];
	}

	int frame = get_current_frame();

	return anim->images[frame];
}

Image *Sprite::get_image(int frame)
{
	Animation *anim = instance->animations[instance->current_animation];
	return anim->images[frame];
}

void Sprite::get_filenames(std::string &json_filename, std::string &image_directory)
{
	json_filename = this->json_filename;
	image_directory = this->image_directory;
}

bool Sprite::is_finished()
{
	Animation *anim = instance->animations[instance->current_animation];
	return instance->elapsed >= anim->total_delays;
}

void Sprite::sync_with(Sprite *sprite, bool match_animation)
{
	if (match_animation) {
		instance->current_animation = sprite->instance->current_animation;
	}
	instance->started = sprite->instance->started;
	instance->elapsed = sprite->instance->elapsed;
}

void Sprite::get_bounds(util::Point<int> &topleft, util::Point<int> &bottomright)
{
	topleft = instance->animations[instance->current_animation]->opaque_topleft;
	bottomright = instance->animations[instance->current_animation]->opaque_bottomright;
}

int Sprite::get_num_frames()
{
	return (int)instance->animations[instance->current_animation]->images.size();
}

std::vector<Uint32> Sprite::get_frame_times()
{
	return instance->animations[instance->current_animation]->delays;
}

Uint32 Sprite::get_elapsed()
{
	return instance->elapsed;
}

void Sprite::update()
{
	std::map<std::string, Loaded_Sprite *>::iterator it;
	it = loaded_sprites.find(image_directory);
	if (it != loaded_sprites.end()) {
		const std::pair<std::string, Loaded_Sprite *> &p = *it;
		update_loaded_sprite(p.first, p.second);
	}
}

Sprite::Animation *Sprite::get_animation(std::string name)
{
	if (instance) {
		std::map<std::string, Animation *>::iterator it = instance->animations.find(name);
		if (it != instance->animations.end()) {
			return it->second;
		}
		else {
			return NULL;
		}
	}
	else {
		return NULL;
	}
}

void Sprite::set_rand_start(bool rand_start)
{
	instance->animations[instance->current_animation]->rand_start = rand_start;
}

Image *Sprite::get_parent_image()
{
	return instance->animations[instance->current_animation]->parent;
}

} // End namespace gfx

} // End namespace noo
