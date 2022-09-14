// These maps are produced by AshEdit: https://github.com/Nooskewl/AshEdit

#include "shim4/error.h"
#include "shim4/gfx.h"
#include "shim4/image.h"
#include "shim4/shader.h"
#include "shim4/shim.h"
#include "shim4/tilemap.h"
#include "shim4/util.h"
#include "shim4/vertex_cache.h"

using namespace noo;

namespace noo {

namespace gfx {

static std::vector<Image *> sheets;

float Tilemap::elapsed;

void Tilemap::static_start()
{
	sheets.clear();
	elapsed = 0.0f;
}

void Tilemap::release_sheets()
{
	for (size_t i = 0; i < sheets.size(); i++) {
		delete sheets[i];
	}
	sheets.clear();
}

void Tilemap::update_all()
{
	elapsed += (1000.0f / shim::logic_rate);
}

Image *Tilemap::get_sheet(int sheet)
{
	return sheets[sheet];
}

static Image *pad(Image *image)
{
	int tiles_w = image->size.w / shim::tile_size;
	int tiles_h = image->size.h / shim::tile_size;

	int needed_w = tiles_w * (shim::tile_size + 2);
	int needed_h = tiles_h * (shim::tile_size + 2);

	bool was_enabled = Image::create_depth_buffer;
	Image::create_depth_buffer = false;
	Image *padded = new Image(util::Size<int>(needed_w, needed_h));
	Image::create_depth_buffer = was_enabled;

	Image *old_target = get_target_image();
	set_target_image(padded);

	image->start_batch();
	
	Vertex_Cache::instance()->maybe_resize_cache(8 * 6 * tiles_w * tiles_h);

	for (int y = 0; y < tiles_h; y++) {
		for (int x = 0; x < tiles_w; x++) {
			float sx = x * shim::tile_size;
			float sy = y * shim::tile_size;
			float dx = x * (shim::tile_size + 2) + 1;
			float dy = y * (shim::tile_size + 2) + 1;
			image->draw_region({sx, sy}, {shim::tile_size, shim::tile_size}, {dx, dy});
			// sides
			image->draw_region({sx, sy}, {shim::tile_size, 1}, {dx, dy-1});
			image->draw_region({sx, sy}, {1, shim::tile_size}, {dx-1, dy});
			image->draw_region({sx, sy+shim::tile_size-1}, {shim::tile_size, 1}, {dx, dy+shim::tile_size});
			image->draw_region({sx+shim::tile_size-1, sy}, {1, shim::tile_size}, {dx+shim::tile_size, dy});
			// corners
			image->draw_region({sx, sy}, {1, 1}, {dx-1, dy-1});
			image->draw_region({sx+shim::tile_size-1, sy}, {1, 1}, {dx+shim::tile_size, dy-1});
			image->draw_region({sx+shim::tile_size-1, sy+shim::tile_size-1}, {1, 1}, {dx+shim::tile_size, dy+shim::tile_size});
			image->draw_region({sx, sy+shim::tile_size-1}, {1, 1}, {dx-1, dy+shim::tile_size});
		}
	}

	image->end_batch();

	set_target_image(old_target);

	return padded;
}

void Tilemap::reload_sheets(bool load_from_filesystem)
{
	for (int i = 0; i < 256; i++) {
		std::string filename = std::string("tiles/tiles" + util::itos(i) + ".tga");
		Image *image;
		try {
			image = new Image(filename, false, load_from_filesystem);
		}
		catch (util::Error &e) {
			/*
			if (i == 0) {
				throw util::LoadError("no tile sheets!");
			}
			else {
			*/
				break;
			//}
		}

		if (load_from_filesystem) {
			// FIXME: need a way to read from texture into buffer so we can read padded image
			// for tilemap optimisation
			sheets.push_back(image);
		}
		else {
			sheets.push_back(pad(image));

			delete image;
		}
	}
}

Tilemap::Tilemap(std::string map_filename, bool load_from_filesystem)
{
	if (load_from_filesystem == false) {
		map_filename = "misc/maps/" + map_filename;
	}

	if (sheets.size() == 0) {
		reload_sheets(load_from_filesystem);
	}

	SDL_RWops *f;
	if (load_from_filesystem) {
		f = SDL_RWFromFile(map_filename.c_str(), "rb");
		if (f == nullptr) {
			throw util::FileNotFoundError("Can't open exe (" + map_filename + ")");
		}
	}
	else {
		f = util::open_file(map_filename, 0);
	}

	(void)SDL_ReadLE32(f); // Skip "DOGO"

	size.w = SDL_ReadLE16(f);
	size.h = SDL_ReadLE16(f);
	num_layers = (unsigned char)util::SDL_fgetc(f);

	layers = new Layer[num_layers];

	solid = new bool *[size.h];

	for (int row = 0; row < size.h; row++) {
		solid[row] = new bool[size.w];
		for (int col = 0; col < size.w; col++) {
			solid[row][col] = ((unsigned char)util::SDL_fgetc(f)) == 1 ? true : false;
		}
	}

	for (int layer = 0; layer < num_layers; layer++) {
		layers[layer].x = new int *[size.h];
		layers[layer].y = new int *[size.h];
		for (int row = 0; row < size.h; row++) {
			layers[layer].x[row] = new int[size.w];
			layers[layer].y[row] = new int[size.w];
			for (int col = 0; col < size.w; col++) {
				int flags = (unsigned char)util::SDL_fgetc(f);
				if (flags & 1) {
					layers[layer].x[row][col] = (unsigned char)util::SDL_fgetc(f);
					layers[layer].y[row][col] = (unsigned char)util::SDL_fgetc(f);
					(void)util::SDL_fgetc(f); // skip sheet, for not it's unused
				}
				else {
					layers[layer].x[row][col] = -1;
					layers[layer].y[row][col] = -1;
				}
			}
		}
	}
	
	util::close_file(f);
}

Tilemap::~Tilemap()
{
	if (layers) {
		for (int layer = 0; layer < num_layers; layer++) {
			for (int row = 0; row < size.h; row++) {
				delete[] layers[layer].x[row];
				delete[] layers[layer].y[row];
			}
			delete[] layers[layer].x;
			delete[] layers[layer].y;
		}
		
		delete[] layers;
	}

	for (int row = 0; row < size.h; row++) {
		delete[] solid[row];
	}
	delete[] solid;
}

int Tilemap::get_num_layers()
{
	return num_layers;
}

util::Size<int> Tilemap::get_size()
{
	return size;
}

bool Tilemap::is_solid(int layer, util::Point<int> position)
{
	if (position.x < 0 || position.y < 0 || position.x >= size.w || position.y >= size.h) {
		return true;
	}

	return solid[position.y][position.x];
}

void Tilemap::set_solid(int layer, util::Point<int> position, util::Size<int> solid_size, bool solid)
{
	for (int y = 0; y < solid_size.h; y++) {
		for (int x = 0; x < solid_size.w; x++) {
			int xx = position.x + x;
			int yy = position.y + y;
			if (xx < 0 || yy < 0 || xx >= size.w || yy >= size.h) {
				continue;
			}
			this->solid[yy][xx] = solid;
		}
	}
}

void Tilemap::swap_tiles(int layer1, int layer2, util::Point<int> topleft, util::Size<int> swap_size)
{
	Layer l_1 = layers[layer1];
	Layer l_2 = layers[layer2];

	for (int y = 0; y < swap_size.h; y++) {
		for (int x = 0; x < swap_size.w; x++) {
			int xx = topleft.x + x;
			int yy = topleft.y + y;
			if (xx < 0 || yy < 0 || xx >= size.w || yy >= size.h) {
				continue;
			}
			int tx = l_2.x[yy][xx];
			int ty = l_2.y[yy][xx];
			l_2.x[yy][xx] = l_1.x[yy][xx];
			l_2.y[yy][xx] = l_1.y[yy][xx];
			l_1.x[yy][xx] = tx;
			l_1.y[yy][xx] = ty;
		}
	}
}

bool Tilemap::collides(int layer, util::Point<int> topleft, util::Point<int> bottomright)
{
	int start_column = topleft.x / shim::tile_size;
	int end_column = bottomright.x / shim::tile_size;
	int start_row = topleft.y / shim::tile_size;
	int end_row = bottomright.y / shim::tile_size;

	start_column = MIN(size.w-1, MAX(0, start_column));
	end_column = MIN(size.w-1, MAX(0, end_column));
	start_row = MIN(size.h-1, MAX(0, start_row));
	end_row = MIN(size.h-1, MAX(0, end_row));

	for (int row = start_row; row <= end_row; row++) {
		for (int column = start_column; column <= end_column; column++) {
			if (solid[row][column]) {
				return true;
			}
		}
	}

	return false;
}

void Tilemap::draw(int start_layer, int end_layer, util::Point<float> position, bool clip_small_tilemaps)
{
#ifdef VERBOSE
	util::debugmsg("drawing layers %d to %d (inclusive)\n", start_layer, end_layer);
#endif

	sheets[0]->start_batch();

	for (int layer = start_layer; layer <= end_layer; layer++) {
		Layer l = layers[layer];

		// Clipping
		util::Size<int> target_size = shim::screen_size;
		util::Size<int> pixel_size = size * shim::tile_size;
		util::Point<int> offset((target_size.w-pixel_size.w)/2, (target_size.h-pixel_size.h)/2);

		util::Point<int> start_tile, end_tile;
		if (position.x < 0) {
			start_tile.x = MIN(size.w - 1, fabs(position.x) / shim::tile_size);
		}
		else {
			start_tile.x = 0;
		}
		if (position.x+pixel_size.w > target_size.w) {
			end_tile.x = MIN(size.w - 1, (fabs(position.x) + target_size.w) / shim::tile_size);
		}
		else {
			end_tile.x = size.w-1;
		}
		if (position.y < 0) {
			start_tile.y = MIN(size.h - 1, fabs(position.y) / shim::tile_size);
		}
		else {
			start_tile.y = 0;
		}
		if (position.y+pixel_size.h > target_size.h) {
			end_tile.y = MIN(size.h - 1, (fabs(position.y) + target_size.h) / shim::tile_size);
		}
		else {
			end_tile.y = size.h-1;
		}

		for (int row = start_tile.y; row <= end_tile.y; row++) {
			for (int col = start_tile.x; col <= end_tile.x; col++) {
				int x = l.x[row][col];
				int y = l.y[row][col];

				if (x < 0 || y < 0) {
					continue;
				}


				util::Point<int> tile = get_animated_tile({x, y});

				float sx = tile.x * (shim::tile_size + 2) + 1;
				float sy = tile.y * (shim::tile_size + 2) + 1;
				float dx = position.x + col * shim::tile_size;
				float dy = position.y + row * shim::tile_size;
				int dw = shim::tile_size;
				int dh = shim::tile_size;

				if (clip_small_tilemaps) {
					if (offset.x > 0) {
						if (dx < offset.x) {
							int diff = offset.x - dx;
							if (diff >= shim::tile_size) {
								continue;
							}
							dx += diff;
							sx += diff;
							dw -= diff;
						}
						if (dx+dw > offset.x+pixel_size.w) {
							int diff = (dx+dw) - (offset.x+pixel_size.w);
							if (diff >= shim::tile_size) {
								continue;
							}
							dw -= diff;
						}
					}
					if (offset.y > 0) {
						if (dy < offset.y) {
							int diff = offset.y - dy;
							if (diff >= shim::tile_size) {
								continue;
							}
							dy += diff;
							sy += diff;
							dh -= diff;
						}
						if (dy+dh > offset.y+pixel_size.h) {
							int diff = (dy+dh) - (offset.y+pixel_size.h);
							if (diff >= shim::tile_size) {
								continue;
							}
							dh -= diff;
						}
					}
				}

				sheets[0]->draw_region({sx, sy}, {dw, dh}, {dx, dy}, 0);
			}
		}
	}

	sheets[0]->end_batch();
}

void Tilemap::draw(int layer, util::Point<float> position, bool clip_small_tilemaps)
{
	draw(layer, layer, position, clip_small_tilemaps);
}

void Tilemap::add_animation_data(Animation_Data data)
{
	animation_data.push_back(data);
}

Tilemap::Animation_Data *Tilemap::get_animation_data(util::Point<int> tile)
{
	for (size_t i = 0; i < animation_data.size(); i++) {
		Animation_Data &a = animation_data[i];
		if (a.topleft.x <= tile.x && a.topleft.y <= tile.y && a.topleft.x+a.size.w > tile.x && a.topleft.y+a.size.h > tile.y) {
			return &a;
		}
	}

	return NULL;
}

util::Point<int> Tilemap::get_animated_tile(util::Point<int> tile)
{
	Animation_Data *a = get_animation_data(tile);
	if (a == NULL) {
		return tile;
	}

	Uint32 length = Uint32((a->frames.size()+1) * a->delay);

	Uint32 mod = Uint32(elapsed) % length;

	int frame = mod / a->delay;

	if (frame == 0) {
		return tile;
	}
	else {
		return a->frames[frame-1] + (tile - a->topleft);
	}
}

bool Tilemap::get_tile(int layer, util::Point<int> position, util::Point<int> &tile_xy, bool &solid)
{
	if (layer < 0 || layer >= num_layers || position.x < 0 || position.y < 0 || position.x >= size.w || position.y >= size.h) {
		return false;
	}

	tile_xy.x = layers[layer].x[position.y][position.x];
	tile_xy.y = layers[layer].y[position.y][position.x];
	solid = this->solid[position.y][position.x];

	return true;
}

bool Tilemap::set_tile(int layer, util::Point<int> position, util::Point<int> tile_xy, bool solid)
{
	if (layer < 0 || layer >= num_layers || position.x < 0 || position.y < 0 || position.x >= size.w || position.y >= size.h) {
		return false;
	}

	layers[layer].x[position.y][position.x] = tile_xy.x;
	layers[layer].y[position.y][position.x] = tile_xy.y;
	this->solid[position.y][position.x] = solid;

	return true;
}

void Tilemap::save(std::string filename)
{
	SDL_RWops *file = SDL_RWFromFile(filename.c_str(), "wb");
	
	util::SDL_fputc('W', file);
	util::SDL_fputc('M', file);
	util::SDL_fputc('2', file);
	util::SDL_fputc('!', file);

	SDL_WriteLE16(file, size.w);
	SDL_WriteLE16(file, size.h);

	util::SDL_fputc(num_layers, file);

	// First optimise

	// Remove completely blank tiles
	for (int row = 0; row < size.h; row++) {
		for (int col = 0; col < size.w; col++) {
			for (int layer = 0; layer < num_layers; layer++) {
				if (layers[layer].x[row][col] >= 0 && layers[layer].y[row][col] >= 0) {
					if (all_clear(layers[layer].x[row][col], layers[layer].y[row][col])) {
						layers[layer].x[row][col] = -1;
						layers[layer].y[row][col] = -1;
					}
				}
			}
		}
	}

	// Remove completely obscured tiles
	for (int row = 0; row < size.h; row++) {
		for (int col = 0; col < size.w; col++) {
			for (int layer = num_layers-1; layer >= 1; layer--) {
				if (layers[layer].x[row][col] >= 0 && layers[layer].y[row][col] >= 0) {
					if (has_alpha(layers[layer].x[row][col], layers[layer].y[row][col]) == false) {
						for (layer = layer-1; layer >= 0; layer--) {
							layers[layer].x[row][col] = -1;
							layers[layer].y[row][col] = -1;
						}
						break;
					}
				}
			}
		}
	}

	// Write solids first
	for (int row = 0; row < size.h; row++) {
		for (int col = 0; col < size.w; col++) {
			util::SDL_fputc(is_solid(-1, {col, row}) ? 1 : 0, file);
		}
	}

	for (int layer = 0; layer < num_layers; layer++) {
		for (int row = 0; row < size.h; row++) {
			for (int col = 0; col < size.w; col++) {
				if (layers[layer].x[row][col] < 0 || layers[layer].y[row][col] < 0) {
					util::SDL_fputc(0, file);
				}
				else {
					util::SDL_fputc(1, file);
					util::SDL_fputc(layers[layer].x[row][col], file);
					util::SDL_fputc(layers[layer].y[row][col], file);
					util::SDL_fputc(0, file); // For now, there is only 1 sheet
				}
			}
		}
	}

	SDL_RWclose(file);
}

bool Tilemap::has_alpha(int col, int row)
{
	unsigned char *data = sheets[0]->get_loaded_data();
	if (data == 0) {
		return true;
	}

	for (int y = 0; y < shim::tile_size; y++) {
		unsigned char *p = data + /*image is flipped*/(sheets[0]->size.h - (row * shim::tile_size + y) - 1) * sheets[0]->size.w * 4 + col * shim::tile_size * 4;
		for (int x = 0; x < shim::tile_size; x++) {
			if (p[3] != 255) {
				return true;
			}
			p += 4;
		}
	}
	
	return false;
}

bool Tilemap::all_clear(int col, int row)
{
	unsigned char *data = sheets[0]->get_loaded_data();
	if (data == 0) {
		return false;
	}

	for (int y = 0; y < shim::tile_size; y++) {
		unsigned char *p = data + /*image is flipped*/(sheets[0]->size.h - (row * shim::tile_size + y) - 1) * sheets[0]->size.w * 4 + col * shim::tile_size * 4;
		for (int x = 0; x < shim::tile_size; x++) {
			if (p[3] != 0) {
				return false;
			}
			p += 4;
		}
	}
	
	return true;
}

} // End namespace gfx

} // End namespace noo
