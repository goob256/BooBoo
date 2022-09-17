// TGA loader taken from http://paulbourke.net/dataformats/tga/

#include "shim4/error.h"
#include "shim4/gfx.h"
#include "shim4/image.h"
#include "shim4/json.h"
#include "shim4/shader.h"
#include "shim4/shim.h"
#include "shim4/util.h"
#include "shim4/vertex_cache.h"

#include "shim4/internal/gfx.h"

#ifdef USE_PNG
#include <png.h>
#endif

#if defined ANDROID || defined IOS
#define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_OES
#endif

using namespace noo;

static inline unsigned char *pixel_ptr(unsigned char *p, int n, bool flip, int w, int h)
{
	if (flip) {
		int x = n % w;
		int y = n / w;
		return p + (w * 4) * (h-1) - (y * w * 4) +  x * 4;
	}
	else {
		return p + n * 4;
	}
}

static inline void pixel_xy(int n, int w, int *x, int *y)
{
	*x = n % w;
	*y = n / w;
}

static inline void maybe_resize_bounds(util::Point<int> &opaque_topleft, util::Point<int> &opaque_bottomright, int x, int y)
{
	if (opaque_topleft.x == -1 || x < opaque_topleft.x) {
		opaque_topleft.x = x;
	}
	if (opaque_topleft.y == -1 || y < opaque_topleft.y) {
		opaque_topleft.y = y;
	}
	if (opaque_bottomright.x == -1 || x > opaque_bottomright.x) {
		opaque_bottomright.x = x;
	}
	if (opaque_bottomright.y == -1 || y > opaque_bottomright.y) {
		opaque_bottomright.y = y;
	}
}

namespace noo {

namespace gfx {

std::vector<Image::Internal *> Image::loaded_images;

static GLuint bound_fbo;
#ifdef _WIN32
static LPDIRECT3DSURFACE9 bound_depth_buffer;
static bool depth_buffer_bound;
#endif
bool Image::dumping_colours;
bool Image::keep_data;
bool Image::save_rle;
bool Image::ignore_palette;
bool Image::create_depth_buffer;
bool Image::create_stencil_buffer;
int Image::d3d_depth_buffer_count;
int Image::d3d_video_texture_count;
int Image::d3d_system_texture_count;
int Image::d3d_surface_level_count;
bool Image::premultiply_alpha;
bool Image::save_rgba;
bool Image::save_palettes;

void Image::static_start()
{
	bound_fbo = 0;
#ifdef _WIN32
	bound_depth_buffer = 0;
	depth_buffer_bound = false;
#endif
	d3d_depth_buffer_count = 0;
	d3d_video_texture_count = 0;
	d3d_system_texture_count = 0;
	d3d_surface_level_count = 0;
	
	util::JSON::Node *root = shim::shim_json->get_root();

	dumping_colours = root->get_nested_bool("shim>gfx>image>dumping_colours", &dumping_colours, false);
	keep_data = root->get_nested_bool("shim>gfx>image>keep_data", &keep_data, false);
	save_rle = root->get_nested_bool("shim>gfx>image>save_rle", &save_rle, false);
	ignore_palette = root->get_nested_bool("shim>gfx>image>ignore_palette", &ignore_palette, false);
	create_depth_buffer = root->get_nested_bool("shim>gfx>image>create_depth_buffer", &create_depth_buffer, false);
	create_stencil_buffer = root->get_nested_bool("shim>gfx>image>create_stencil_buffer", &create_stencil_buffer, false);
	premultiply_alpha = root->get_nested_bool("shim>gfx>image>premultiply_alpha", &premultiply_alpha, true);
	save_rgba = root->get_nested_bool("shim>gfx>image>save_rgba", &save_rgba, false);
	save_palettes = root->get_nested_bool("shim>gfx>image>save_palettes", &save_palettes, true);
}

void Image::release_all(bool include_managed)
{
	util::infomsg("Releasing %d textures...\n", loaded_images.size());
	for (size_t i = 0; i < loaded_images.size(); i++) {
#ifdef _WIN32
		if (shim::opengl == true || (include_managed && loaded_images[i]->has_render_to_texture == false)) {
#endif
			loaded_images[i]->release();
#ifdef _WIN32
		}
		else {
			loaded_images[i]->unbind();
		}
#endif
	}
}

void Image::reload_all(bool include_managed)
{
	for (size_t i = 0; i < loaded_images.size(); i++) {
#ifdef _WIN32
		if (shim::opengl == true || (include_managed && loaded_images[i]->has_render_to_texture == false)) {
#endif
			loaded_images[i]->reload(false);
#ifdef _WIN32
		}
#endif
	}
}

int Image::get_unfreed_count()
{
	for (size_t i = 0; i < loaded_images.size(); i++) {
		util::infomsg("Unfreed: %s.\n", loaded_images[i]->filename.c_str());
	}
	return (int)loaded_images.size();
}

void Image::audit()
{
	if (shim::opengl == false) {
		util::debugmsg("d3d_depth_buffer_count=%d\n", d3d_depth_buffer_count);
		util::debugmsg("d3d_video_texture_count=%d\n", d3d_video_texture_count);
		util::debugmsg("d3d_system_texture_count=%d\n", d3d_system_texture_count);
		util::debugmsg("d3d_surface_level_count=%d\n", d3d_surface_level_count);
	}
}

static void read_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
    SDL_RWops *f = (SDL_RWops *)png_get_io_ptr(png_ptr);
    if ((png_uint_32)SDL_RWread(f, data, length, 1) != 1) {
    	throw util::LoadError("Error loading PNG");
    }
}

#ifdef USE_PNG
unsigned char *Image::read_png(std::string filename, util::Size<int> &out_size, SDL_Colour *out_palette, util::Point<int> *opaque_topleft, util::Point<int> *opaque_bottomright, bool *has_alpha, bool load_from_filesystem)
{
	FILE *fp;
	SDL_RWops *file;
	int sz;
	int number_to_check = 8;

	if (load_from_filesystem) {
		fp = fopen(filename.c_str(), "rb");
		if (!fp){
			return nullptr;
		}
		unsigned char header[9];
		fread(header, 1, number_to_check, fp);
		int is_png = !png_sig_cmp(header, 0, number_to_check);
		if (!is_png){
			return nullptr;
		}
	}
	else {
		file = util::open_file(filename, &sz);
		if (!file){
			return nullptr;
		}
		unsigned char header[9];
		SDL_RWread(file, header, number_to_check, 1);
		int is_png = !png_sig_cmp(header, 0, number_to_check);
		if (!is_png){
			return nullptr;
		}
	}

	// Create png struct pointer
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		return nullptr;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return nullptr;
	}
	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		return false;
	}

	if (load_from_filesystem) {
		png_init_io(png_ptr, fp);
	}
	else {
	    png_set_read_fn(png_ptr, file, (png_rw_ptr)read_data);
	}

	png_set_sig_bytes(png_ptr, number_to_check);

	// 3. Read png info
	png_read_info(png_ptr, info_ptr);

	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type, compression_type, filter_method;

	bit_depth = png_get_bit_depth (png_ptr, info_ptr);
	color_type = png_get_color_type (png_ptr, info_ptr);

	png_set_strip_16(png_ptr);
	png_set_expand(png_ptr);
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb (png_ptr);
	}
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
	       //png_set_gray_1_2_4_to_8 (png_ptr);
	}
	if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS)) {
	          png_set_tRNS_to_alpha (png_ptr);
	}
	if (bit_depth == 16) {
		png_set_strip_16 (png_ptr);
	}
	else if (bit_depth < 8) {
	      png_set_packing (png_ptr);
	}

	png_read_update_info (png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

	png_bytepp row_pointers = (png_bytepp)png_malloc(png_ptr, sizeof(png_bytepp) * height);
	for (int i = 0; i < height; i++) {
		row_pointers[i] = (png_bytep)png_malloc(png_ptr, width * 4);
	}
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_read_image(png_ptr, row_pointers);
	png_read_end(png_ptr, end_info);

	unsigned char *bytes = new unsigned char[width * height * 4];

	if (color_type == PNG_COLOR_TYPE_RGB) {
		for (int y = 0; y < height; y++) {
			unsigned char *p = bytes + (height-y-1)*width*4;
			unsigned char *p2 = row_pointers[y];
			for (int i = 0; i < width; i++) {
				int r = *p2++;
				int g = *p2++;
				int b = *p2++;
				int a = 255;
				*p++ = r;
				*p++ = g;
				*p++ = b;
				*p++ = a;
			}
		}
	}
	else if (color_type == PNG_COLOR_TYPE_RGBA) {
		for (int i = 0; i < height; i++) {
			memcpy(bytes+(height-i-1)*width*4, row_pointers[i], width * 4);
		}
	}
	else {
		return nullptr;
	}

	for (int i = 0; i < height; i++) {
		png_free(png_ptr, row_pointers[i]);
	}
	png_free(png_ptr, row_pointers);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	out_size.w = width;
	out_size.h = height;

	// FIXME opaque

	return bytes;
}
#endif

unsigned char *Image::read_tga(std::string filename, util::Size<int> &out_size, SDL_Colour *out_palette, util::Point<int> *opaque_topleft, util::Point<int> *opaque_bottomright, bool *has_alpha, bool load_from_filesystem)
{
	SDL_RWops *file;
	if (load_from_filesystem) {
		file = SDL_RWFromFile(filename.c_str(), "rb");
	}
	else {
		file = util::open_file(filename, 0);
	}

	if (file == 0) {
		return 0;
	}

	int n = 0, i = 0, j;
	int bytes2read;
	unsigned char p[5];
	TGA_Header header;
	unsigned char *pixels;
	int pixel_x, pixel_y;
	bool alpha;

	/* Display the header fields */
	header.idlength = util::SDL_fgetc(file);
	header.colourmaptype = util::SDL_fgetc(file);
	header.datatypecode = util::SDL_fgetc(file);
	header.colourmaporigin = SDL_ReadLE16(file);
	header.colourmaplength = SDL_ReadLE16(file);
	header.colourmapdepth = util::SDL_fgetc(file);
	header.x_origin = SDL_ReadLE16(file);
	header.y_origin = SDL_ReadLE16(file);
	header.width = SDL_ReadLE16(file);
	header.height = SDL_ReadLE16(file);
	header.bitsperpixel = util::SDL_fgetc(file);
	header.imagedescriptor = util::SDL_fgetc(file);

	int w, h;
	out_size.w = w = header.width;
	out_size.h = h = header.height;

	if (has_alpha) {
		*has_alpha = false;
	}

	try {
		/* Allocate space for the image */
		if ((pixels = new unsigned char[header.width*header.height*4]) == 0) {
			throw util::MemoryError("malloc of image failed");
		}

		/* What can we handle */
		if (header.datatypecode != 1 && header.datatypecode != 2 && header.datatypecode != 9 && header.datatypecode != 10) {
			throw util::LoadError("can only handle image type 1, 2, 9 and 10");
		}
		if (header.bitsperpixel != 8 && header.bitsperpixel != 16 && header.bitsperpixel != 24 && header.bitsperpixel != 32) {
			throw util::LoadError("can only handle pixel depths of 8, 16, 24 and 32");
		}
		if (header.colourmaptype != 0 && header.colourmaptype != 1) {
			throw util::LoadError("can only handle colour map types of 0 and 1");
		}

		/* Skip over unnecessary stuff */
		SDL_RWseek(file, header.idlength, RW_SEEK_CUR);

		/* Read the palette if there is one */
		if (header.colourmaptype == 1) {
			if (header.colourmapdepth != 24) {
				throw util::LoadError("can't handle anything but 24 bit palettes");
			}
			if (header.bitsperpixel != 8) {
				throw util::LoadError("can only read 8 bpp paletted images");
			}
			int skip = header.colourmaporigin * (header.colourmapdepth / 8);
			SDL_RWseek(file, skip, RW_SEEK_CUR);
			// We can only read 256 colour palettes max, skip the rest
			int size = MIN(header.colourmaplength-skip, 256);
			skip = (header.colourmaplength - size) * (header.colourmapdepth / 8);
			for (i = 0; i < size; i++) {
				header.palette[i].b = util::SDL_fgetc(file);
				header.palette[i].g = util::SDL_fgetc(file);
				header.palette[i].r = util::SDL_fgetc(file);
			}
			SDL_RWseek(file, skip, RW_SEEK_CUR);
		}
		else {
			// Skip the palette on truecolour images
			SDL_RWseek(file, (header.colourmapdepth / 8) * header.colourmaplength, RW_SEEK_CUR);
		}

		bool flip = (header.imagedescriptor & 0x20) != 0;

		/* Read the image */
		bytes2read = header.bitsperpixel / 8;
		while (n < header.width * header.height) {
			if (header.datatypecode == 1 || header.datatypecode == 2) {                     /* Uncompressed */
				if (SDL_RWread(file, p, 1, bytes2read) != (size_t)bytes2read) {
					delete[] pixels;
					throw util::LoadError("unexpected end of file at pixel " + util::itos(i));
				}
				if (merge_bytes(pixel_ptr(pixels, n, flip, w, h), p, bytes2read, &header, &alpha) == false) {
					if (opaque_topleft != 0 && opaque_bottomright != 0) {
						pixel_xy(n, w, &pixel_x, &pixel_y);
						maybe_resize_bounds(*opaque_topleft, *opaque_bottomright, pixel_x, pixel_y);
					}
				}
				n++;
				if (alpha && has_alpha) {
					*has_alpha = true;
				}
			}
			else if (header.datatypecode == 9 || header.datatypecode == 10) {             /* Compressed */
				if (SDL_RWread(file, p, 1, bytes2read+1) != (size_t)bytes2read+1) {
					delete[] pixels;
					throw util::LoadError("unexpected end of file at pixel " + util::itos(i));
				}
				j = p[0] & 0x7f;
				if (merge_bytes(pixel_ptr(pixels, n, flip, w, h), &(p[1]), bytes2read, &header, &alpha) == false) {
					if (opaque_topleft != 0 && opaque_bottomright != 0) {
						pixel_xy(n, w, &pixel_x, &pixel_y);
						maybe_resize_bounds(*opaque_topleft, *opaque_bottomright, pixel_x, pixel_y);
					}
				}
				n++;
				if (alpha && has_alpha) {
					*has_alpha = true;
				}
				if (p[0] & 0x80) {         /* RLE chunk */
					for (i = 0; i < j; i++) {
						if (merge_bytes(pixel_ptr(pixels, n, flip, w, h), &(p[1]), bytes2read, &header, &alpha) == false) {
							if (opaque_topleft != 0 && opaque_bottomright != 0) {
								pixel_xy(n, w, &pixel_x, &pixel_y);
								maybe_resize_bounds(*opaque_topleft, *opaque_bottomright, pixel_x, pixel_y);
							}
						}
						n++;
						if (alpha && has_alpha) {
							*has_alpha = true;
						}
					}
				}
				else {                   /* Normal chunk */
					for (i = 0; i < j; i++) {
						if (SDL_RWread(file, p, 1, bytes2read) != (size_t)bytes2read) {
							delete[] pixels;
							throw util::LoadError("unexpected end of file at pixel " + util::itos(i));
						}
						if (merge_bytes(pixel_ptr(pixels, n, flip, w, h), p, bytes2read, &header, &alpha) == false) {
							if (opaque_topleft != 0 && opaque_bottomright != 0) {
								pixel_xy(n, w, &pixel_x, &pixel_y);
								maybe_resize_bounds(*opaque_topleft, *opaque_bottomright, pixel_x, pixel_y);
							}
						}
						n++;
						if (alpha && has_alpha) {
							*has_alpha = true;
						}
					}
				}
			}
		}
	}
	catch (util::Error &) {
		util::close_file(file);
		throw;
	}

	util::close_file(file);

	if (out_palette != 0) {
		memcpy(out_palette, header.palette, 256 * 3);
	}

	return pixels;
}

unsigned char *Image::read_texture(gfx::Image *image)
{
	unsigned char *buf = new unsigned char[image->size.w*image->size.h*4];

#ifdef ANDROID
	return buf;
#else
	if (shim::opengl) {
		glActiveTexture_ptr(GL_TEXTURE0);
		PRINT_GL_ERROR("glActiveTexture\n");

		glBindTexture_ptr(GL_TEXTURE_2D, image->internal->texture);
		PRINT_GL_ERROR("glBindTexture\n");

		glGetTexImage_ptr(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);

		for (int y = 0; y < image->size.h; y++) {
			unsigned char *p = buf+y*image->size.w*4;
			for (int x = 0; x < image->size.w; x++) {
				int r = p[0];
				int g = p[1];
				int b = p[2];
				int a = p[3];
				p += 4;
			}
		}
	}
#ifdef _WIN32
	else {
		IDirect3DSurface9 *sys_surf;
		IDirect3DSurface9 *vid_surf;
		if (image->internal->system_texture->GetSurfaceLevel(0, &sys_surf) == D3D_OK) {
			if (image->internal->video_texture->GetSurfaceLevel(0, &vid_surf) == D3D_OK) {
				if (shim::d3d_device->GetRenderTargetData(vid_surf, sys_surf) == D3D_OK) {
					D3DLOCKED_RECT locked_rect;
					if (sys_surf->LockRect(&locked_rect, 0, D3DLOCK_READONLY) == D3D_OK) {
						for (int y = 0; y < image->size.h; y++) {
							for (int x = 0; x < image->size.w; x++) {
								unsigned char *p = (unsigned char *)locked_rect.pBits+y*locked_rect.Pitch+x*4;
								int b, g, r, a;
								b = *p++;
								g = *p++;
								r = *p++;
								a = *p++;
								buf[(image->size.h-y-1)*image->size.w*4+x*4+0] = r;
								buf[(image->size.h-y-1)*image->size.w*4+x*4+1] = g;
								buf[(image->size.h-y-1)*image->size.w*4+x*4+2] = b;
								buf[(image->size.h-y-1)*image->size.w*4+x*4+3] = a;
							}
						}
						sys_surf->UnlockRect();
					}
				}
				sys_surf->Release();
				vid_surf->Release();
			}
			else {
				sys_surf->Release();
			}
		}
	}
#endif
#endif

	return buf;
}

bool Image::merge_bytes(unsigned char *pixel, unsigned char *p, int bytes, TGA_Header *header, bool *alpha)
{
	if (header->colourmaptype == 1) {
		SDL_Colour *colour;
		if (ignore_palette) {
			colour = &shim::palette[*p];
		}
		else {
			colour = &header->palette[*p];
		}
		// Magic pink
		// Paletted
		if (colour->r == 255 && colour->g == 0 && colour->b == 255) {
			// transparent
			*pixel++ = 0;
			*pixel++ = 0;
			*pixel++ = 0;
			*pixel++ = 0;
			return true;
		}
		else {
			*pixel++ = colour->r;
			*pixel++ = colour->g;
			*pixel++ = colour->b;
			*pixel++ = 255;
		}
		*alpha = false;
	}
	else {
		if (bytes == 4) {
			if (premultiply_alpha) {
				float a = p[3] / 255.0f;
				*pixel++ = (unsigned char)(p[2] * a);
				*pixel++ = (unsigned char)(p[1] * a);
				*pixel++ = (unsigned char)(p[0] * a);
			}
			else {
				*pixel++ = p[2];
				*pixel++ = p[1];
				*pixel++ = p[0];
			}
			*pixel++ = p[3];
			if (p[3] != 255 && p[3] != 0) {
				*alpha = true;
			}
			else {
				*alpha = false;
			}
			return p[3] == 0;
		}
		else if (bytes == 3) {
			*pixel++ = p[2];
			*pixel++ = p[1];
			*pixel++ = p[0];
			*pixel++ = 255;
			*alpha = false;
		}
		else if (bytes == 2) {
			*pixel++ = (p[1] & 0x7c) << 1;
			*pixel++ = ((p[1] & 0x03) << 6) | ((p[0] & 0xe0) >> 2);
			*pixel++ = (p[0] & 0x1f) << 3;
			*pixel++ = (p[1] & 0x80);
			*alpha = false;
			return (p[1] & 0x80) == 0;
		}
	}

	return false;
}

Image::Image(std::string filename, bool is_absolute_path, bool load_from_filesystem) :
	batching(false),
	flipped(true),
	data_to_destroy(0)
{
	if (is_absolute_path == false && load_from_filesystem == false) {
		filename = "gfx/images/" + filename;
	}

	this->filename = filename;

	reload(load_from_filesystem);
}

Image::Image(Uint8 *data, util::Size<int> size, bool destroy_data) :
	filename("--NOT LOADED--"),
	size(size),
	batching(false),
	flipped(true)
{
	if (destroy_data) {
		data_to_destroy = data;
	}
	else {
		data_to_destroy = 0;
	}

	try {
		internal = new Internal(data, size);
	}
	catch (util::Error &) {
		delete[] data;
		throw;
	}

	internal->has_alpha = false;
	Uint8 *p = data;
	for (int i = 0; i < size.w * size.h; i++) {
		if (p[3] != 255) {
			internal->has_alpha = true;
			break;
		}
		p += 4;
	}
}

Image::Image(SDL_Surface *surface) :
	filename("--NOT LOADED--"),
	batching(false),
	flipped(true),
	data_to_destroy(0)
{
	unsigned char *pixels;
	unsigned char *packed = 0;
	SDL_Surface *tmp = 0;
	SDL_Surface *fetch;

	if (surface->format->format == SDL_PIXELFORMAT_RGBA8888) {
		fetch = surface;
	}
	else {
		SDL_PixelFormat format;
		format.format = SDL_PIXELFORMAT_RGBA8888;
		format.palette = 0;
		format.BitsPerPixel = 32;
		format.BytesPerPixel = 4;
		format.Rmask = 0xff;
		format.Gmask = 0xff00;
		format.Bmask = 0xff0000;
		format.Amask = 0xff000000;
		tmp = SDL_ConvertSurface(surface, &format, 0);
		if (tmp == 0) {
			throw util::Error("SDL_ConvertSurface returned 0");
		}
		fetch = tmp;
	}

	if (fetch->pitch != fetch->w * 4) {
		// must be packed
		packed = new unsigned char[fetch->w * 4 * fetch->h];
		for (int y = 0; y < fetch->h; y++) {
			memcpy(packed + (y * fetch->w * 4), ((unsigned char *)fetch->pixels) + y * fetch->pitch, fetch->w * 4);
		}
		pixels = packed;
	}
	else {
		pixels = (unsigned char *)fetch->pixels;
	}

	size = {surface->w, surface->h};

	for (int y = 0; y < size.h; y++) {
		for (int x = 0; x < size.w; x++) {
			unsigned char *p = (unsigned char *)(pixels + y * (size.w * 4) + x * 4);
				float a = p[3] / 255.0f;
				p[0] *= a;
				p[1] *= a;
				p[2] *= a;
		}
	}

	try {
		internal = new Internal(pixels, size);
	}
	catch (util::Error &) {
		if (tmp) SDL_FreeSurface(tmp);
		throw;
	}

	internal->has_alpha = false;
	Uint8 *p = pixels;
	for (int i = 0; i < size.w * size.h; i++) {
		if (p[3] != 255) {
			internal->has_alpha = true;
			break;
		}
		p += 4;
	}

	if (tmp) {
		SDL_FreeSurface(tmp);
	}

	if (packed) {
		delete[] packed;
	}
}

Image::Image(util::Size<int> size) :
	filename("--NOT LOADED--"),
	size(size),
	batching(false),
	flipped(false),
	data_to_destroy(0)
{
	unsigned char *pixels = (unsigned char *)calloc(1, size.w * size.h * 4);

	try {
		internal = new Internal(pixels, size, true); // support render to texture
	}
	catch (util::Error &) {
		free(pixels);
		throw;
	}

	internal->has_alpha = true;

	free(pixels);
}

Image::Image(util::Size<int> size, unsigned char *pixels) :
	filename("--NOT LOADED--"),
	size(size),
	batching(false),
	flipped(false),
	data_to_destroy(0)
{
	try {
		internal = new Internal(pixels, size, true); // support render to texture
	}
	catch (util::Error &) {
		free(pixels);
		throw;
	}

	internal->has_alpha = true;
}

Image::Image(Image *parent, util::Point<int> offset, util::Size<int> size) :
	filename("--NOT LOADED--"),
	size(size),
	batching(false),
	flipped(true),
	data_to_destroy(0)
{
	internal = new Internal;
	internal->parent = parent;
	internal->offset = offset;

	internal->has_alpha = parent->internal->has_alpha; // FIXME: not always true

	Image::Internal *parent_internal = parent->internal;

	for (int y = 0; y < size.h; y++) {
		for (int x = 0; x < size.w; x++) {
			if (parent_internal->is_transparent(util::Point<int>(x, y) + offset) == false) {
				maybe_resize_bounds(internal->opaque_topleft, internal->opaque_bottomright, x, y);
			}
		}
	}
}

Image::~Image()
{
	release();
	delete[] data_to_destroy;
}

void Image::release()
{
	if (filename == "--NOT LOADED--") {
		internal->unbind();
		delete internal;
		internal = 0;
		return;
	}

	for (size_t i = 0; i < loaded_images.size(); i++) {
		Internal *ii = loaded_images[i];
		if (ii->filename == filename) {
			ii->refcount--;
			if (ii->refcount == 0) {
				internal->unbind();
				loaded_images.erase(loaded_images.begin()+i);
				delete ii;
				return;
			}
		}
	}
}

void Image::reload(bool load_from_filesystem)
{
	if (filename == "--NOT LOADED--") {
		return;
	}

	for (size_t i = 0; i < loaded_images.size(); i++) {
		Internal *ii = loaded_images[i];
		if (ii->filename == filename) {
			ii->refcount++;
			internal = ii;
			size = internal->size;
			return;
		}
	}

	internal = new Internal(filename, keep_data, false, load_from_filesystem);
	size = internal->size;
	loaded_images.push_back(internal);
}

bool Image::save(std::string filename)
{
	bool _save_rgba = internal->has_alpha || save_rgba; // FIXME: should be able to force NOT saving RGBA

	unsigned char *loaded_data = internal->loaded_data;

	unsigned char header[] = {
		(unsigned char)0x00, // idlength
		(unsigned char)0x01, // colourmap type 1 == palette
		(unsigned char)(save_rle ? 0x09 : 0x01),
		(unsigned char)0x00, (unsigned char)0x00, // colourmap origin (little endian)
		(unsigned char)0x00, save_palettes ? (unsigned char)0x01 : (unsigned char)0x00, // # of palette entries
		(unsigned char)0x18, // colourmap depth
		(unsigned char)0x00, (unsigned char)0x00, // x origin
		(unsigned char)0x00, (unsigned char)0x00, // y origin
		(unsigned char)(size.w & 0xff), (unsigned char)((size.w >> 8) & 0xff), // width
		(unsigned char)(size.h & 0xff), (unsigned char)((size.h >> 8) & 0xff), // height
		(unsigned char)0x08, // bits per pixel
		(unsigned char)0x00 // image descriptor
	};

	if (_save_rgba) {
		header[1] = 0;
		header[2] = 2;
		header[5] = 0;
		header[6] = 0;
		header[7] = 0;
		header[16] = 32;
		header[17] = 8;
	}

	int header_size = 18;

	SDL_RWops *file = SDL_RWFromFile(filename.c_str(), "wb");
	if (file == 0) {
		throw util::Error("Couldn't open " + filename + " for writing");
	}

	for (int i = 0; i < header_size; i++) {
		if (util::SDL_fputc(header[i], file) == EOF) {
			throw util::Error("Write error writing to " + filename);
		}
	}

	if (_save_rgba == false) {
		if (save_palettes) {
			for (int i = 0; i < 256; i++) {
				if (util::SDL_fputc(shim::palette[i].b, file) == EOF) {
					throw util::Error("Write error writing to " + filename);
				}
				if (util::SDL_fputc(shim::palette[i].g, file) == EOF) {
					throw util::Error("Write error writing to " + filename);
				}
				if (util::SDL_fputc(shim::palette[i].r, file) == EOF) {
					throw util::Error("Write error writing to " + filename);
				}
			}
		}

		#define R(n) *(pixel_ptr(loaded_data, n, false, size.w, size.h)+0)
		#define G(n) *(pixel_ptr(loaded_data, n, false, size.w, size.h)+1)
		#define B(n) *(pixel_ptr(loaded_data, n, false, size.w, size.h)+2)

		if (save_rle) {
			for (int i = 0; i < size.w * size.h;) {
				int j, count;
				int next_line = i - (i % size.w) + size.w - 1;
				for (j = i, count = 0; j < size.w * size.h - 1 && j < next_line && count < 127; j++, count++) {
					if (R(j) != R(j+1) || G(j) != G(j+1) || B(j) != B(j+1)) {
						break;
					}
				}
				int run_length = j - i + 1;
				if (run_length > 1) {
					util::SDL_fputc((run_length-1) | 0x80, file);
					util::SDL_fputc(find_colour_in_palette(&R(i)), file);
				}
				else {
					for (j = i, count = 0; j < size.w * size.h - 1 && j < next_line && count < 127; j++, count++) {
						if (R(j) == R(j+1) && G(j) == G(j+1) && B(j) == B(j+1)) {
							break;
						}
					}
					run_length = j - i + 1;
					// I noticed PSP never stores a non-run of 2 pixels, and this saves some space usually, so we do the same
					if (run_length == 2) {
						run_length--;
					}
					util::SDL_fputc((run_length-1), file);
					util::SDL_fputc(find_colour_in_palette(&R(i)), file);
					for (j = 0; j < run_length-1; j++) {
						util::SDL_fputc(find_colour_in_palette(&R(i+j+1)), file);
					}
				}
				i += run_length;
			}
		}
		else {
			for (int i = 0; i < size.w * size.h; i++) {
				util::SDL_fputc(find_colour_in_palette(&R(i)), file);
			}
		}
	}
	else {
		unsigned char *tmp = new unsigned char[size.w * size.h * 4];
		unsigned char *p = tmp;
		unsigned char *p2 = loaded_data;
		for (int i = 0; i < size.w * size.h; i++) {
			unsigned char r = *p2++;
			unsigned char g = *p2++;
			unsigned char b = *p2++;
			unsigned char a = *p2++;
			*p++ = b;
			*p++ = g;
			*p++ = r;
			*p++ = a;
		}
		SDL_RWwrite(file, tmp, size.w * size.h * 4, 1);
		delete[] tmp;
	}

	return true;
}

unsigned char Image::find_colour_in_palette(unsigned char *p)
{
	if (p[3] == 0) {
		return 0;
	}

	for (unsigned int i = 0; i < 256; i++) {
		if (p[0] == shim::palette[i].r && p[1] == shim::palette[i].g && p[2] == shim::palette[i].b) {
			return i;
		}
	}

	util::errormsg("Error: colour %d,%d,%d not found!\n", p[0], p[1], p[2]);

	return 0;
}

void Image::set_target()
{
	if (shim::opengl) {
		bound_fbo = internal->fbo;
		glBindFramebuffer_ptr(GL_FRAMEBUFFER, internal->fbo);
		glViewport_ptr(0, 0, size.w, size.h);
		glDisable_ptr(GL_SCISSOR_TEST);
	}
#ifdef _WIN32
	else {
		util::verbosemsg("device render_target->Release=%d\n", internal::gfx_context.render_target->Release());

		if (internal->video_texture->GetSurfaceLevel(0, &internal->render_target) != D3D_OK) {
			util::infomsg("Image::set_target: Unable to get texture surface level\n");
			return;
		}
		d3d_surface_level_count++;

		if (shim::d3d_device->SetRenderTarget(0, internal->render_target) != D3D_OK) {
			util::infomsg("Image::set_target: Unable to set render target to texture surface\n");
			util::verbosemsg("Image::set_target (failure), render_target->Release=%d\n", internal->render_target->Release());
			return;
		}

		if (internal->depth_stencil_buffer) {
			shim::d3d_device->SetDepthStencilSurface(internal->depth_stencil_buffer);
			bound_depth_buffer = internal->depth_stencil_buffer;
		}
		else {
			shim::d3d_device->SetDepthStencilSurface(0);
			bound_depth_buffer = 0;
		}
		depth_buffer_bound = true;

		D3DVIEWPORT9 viewport;
		viewport.MinZ = 0;
		viewport.MaxZ = 1;
		viewport.X = 0;
		viewport.Y = 0;
		viewport.Width = size.w;
		viewport.Height = size.h;
		shim::d3d_device->SetViewport(&viewport);

		shim::d3d_device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	}
#endif

	// Set an ortho projection the size of the image
	if (this == (Image *)internal::gfx_context.work_image) {
		// mimic the real backbuffer
		set_default_projection(shim::real_screen_size, shim::screen_offset, shim::scale);
		update_projection();
		// also set the default screen scissor
		gfx::unset_scissor();
	}
	else {
		glm::mat4 modelview = glm::mat4();
		glm::mat4 proj = glm::ortho(0.0f, (float)size.w, (float)size.h, 0.0f);
		set_matrices(modelview, proj);
		update_projection();
	}
}

void Image::release_target()
{
	if (shim::opengl) {
		bound_fbo = 0;
#ifdef IOS
		//glBindRenderbuffer_ptr(GL_RENDERBUFFER, internal::gfx_context.colorbuffer); // don't know if this is needed
		glBindFramebuffer_ptr(GL_FRAMEBUFFER, internal::gfx_context.framebuffer);
#else
		glBindFramebuffer_ptr(GL_FRAMEBUFFER, 0);
#endif
	}
#ifdef _WIN32
	else {
		if (internal->render_target != 0) {
			util::verbosemsg("release_target (%p), render_target->Release=%d\n", this, internal->render_target->Release());
			internal->render_target = 0;
			d3d_surface_level_count--;
		}

		if (shim::d3d_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &internal::gfx_context.render_target) != D3D_OK) {
			util::infomsg("GetBackBuffer failed.\n");
		}

		if (shim::d3d_device->SetRenderTarget(0, internal::gfx_context.render_target) != D3D_OK) {
			util::infomsg("Image::release_target: Unable to set render target to backbuffer.\n");
		}

		if (internal::gfx_context.depth_stencil_buffer) {
			shim::d3d_device->SetDepthStencilSurface(internal::gfx_context.depth_stencil_buffer);
			bound_depth_buffer = internal::gfx_context.depth_stencil_buffer;
			depth_buffer_bound = true;
		}
	}
#endif

	set_screen_size(shim::real_screen_size); // this sets the viewport and scissor, updates projection
}

void Image::get_bounds(util::Point<int> &topleft, util::Point<int> &bottomright)
{
	topleft = internal->opaque_topleft;
	bottomright = internal->opaque_bottomright;
}

void Image::set_bounds(util::Point<int> topleft, util::Point<int> bottomright)
{
	internal->opaque_topleft = topleft;
	internal->opaque_bottomright = bottomright;
}

void Image::destroy_data()
{
	if (internal != 0) {
		internal->destroy_data();
	}
}

Image *Image::get_root()
{
	gfx::Image *root = this;
	while (root && root->internal->parent != 0) {
		root = root->internal->parent;
	}
	return root;
}

unsigned char *Image::get_loaded_data()
{
	return internal->loaded_data;
}

void Image::start_batch(bool repeat)
{
	Image *root = get_root();
	Vertex_Cache::instance()->start(root, repeat);
	root->batching = true;
}

void Image::end_batch()
{
	Vertex_Cache::instance()->end();
	get_root()->batching = false;
}

void Image::stretch_region_tinted_repeat(SDL_Colour tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, util::Size<int> dest_size, int flags)
{
	SDL_Colour colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = tint;

	int wt = dest_size.w / source_size.w;
	if (dest_size.w % source_size.w != 0) {
		wt++;
	}
	int ht = dest_size.h / source_size.h;
	if (dest_size.h % source_size.h != 0) {
		ht++;
	}

	bool was_batching = get_root()->batching;
	if (was_batching == false) start_batch();

	int drawn_h = 0;
	for (int y = 0; y < ht; y++) {
		int drawn_w = 0;
		int h = source_size.h;
		if (dest_size.h - drawn_h < h) {
			h = dest_size.h- drawn_h;
		}
		for (int x = 0; x < wt; x++) {
			int w = source_size.w;
			if (dest_size.w - drawn_w < w) {
				w = dest_size.w - drawn_w;
			}
			util::Size<int> sz(w, h);
			Vertex_Cache::instance()->cache(colours, source_position+internal->offset, sz, {dest_position.x + x * source_size.w, dest_position.y + y * source_size.h}, sz, flags);
			drawn_w += w;
		}
		drawn_h += h;
	}

	if (was_batching == false) end_batch();
}

void Image::stretch_region_tinted(SDL_Colour tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, util::Size<int> dest_size, int flags)
{
	SDL_Colour colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = tint;
	bool was_batching = get_root()->batching;
	if (was_batching == false) start_batch();
	Vertex_Cache::instance()->cache(colours, source_position+internal->offset, source_size, dest_position, dest_size, flags);
	if (was_batching == false) end_batch();
}

void Image::stretch_region(util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, util::Size<int> dest_size, int flags)
{
	stretch_region_tinted(shim::white, source_position, source_size, dest_position, dest_size, flags);
}

void Image::draw_region_lit_z_range(SDL_Colour colours[4], util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z_top, float z_bottom, int flags)
{
	bool was_batching = get_root()->batching;
	if (was_batching == false) start_batch();
	Vertex_Cache::instance()->cache_z_range(colours, source_position+internal->offset, source_size, dest_position, z_top, z_bottom, source_size, flags);
	if (was_batching == false) end_batch();
}

void Image::draw_region_lit_z(SDL_Colour colours[4], util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z, int flags)
{
	draw_region_lit_z_range(colours, source_position, source_size, dest_position, z, z, flags);
}

void Image::draw_region_tinted_z_range(SDL_Colour tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z_top, float z_bottom, int flags)
{
	SDL_Colour colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = tint;
	draw_region_lit_z_range(colours, source_position, source_size, dest_position, z_top, z_bottom, flags);
}

void Image::draw_region_tinted_z(SDL_Colour tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z, int flags)
{
	draw_region_tinted_z_range(tint, source_position, source_size, dest_position, z, z, flags);
}

void Image::draw_region_tinted(SDL_Colour tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, int flags)
{
	SDL_Colour colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = tint;
	draw_region_lit_z_range(colours, source_position, source_size, dest_position, 0.0f, 0.0f, flags);
}

void Image::draw_region_z_range(util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z_top, float z_bottom, int flags)
{
	SDL_Colour colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = shim::white;
	draw_region_lit_z_range(colours, source_position, source_size, dest_position, z_top, z_bottom, flags);
}

void Image::draw_region_z(util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z, int flags)
{
	draw_region_tinted_z(shim::white, source_position, source_size, dest_position, z, flags);
}

void Image::draw_region(util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, int flags)
{
	draw_region_z(source_position, source_size, dest_position, 0.0f, flags);
}

void Image::draw_z(util::Point<float> dest_position, float z, int flags)
{
	draw_region_z({0.0f, 0.0f}, size, dest_position, z, flags);
}

void Image::draw_tinted(SDL_Colour tint, util::Point<float> dest_position, int flags)
{
	draw_region_tinted(tint, {0.0f, 0.0f}, size, dest_position, flags);
}

void Image::draw(util::Point<float> dest_position, int flags)
{
	draw_z(dest_position, 0.0f, flags);
}

void Image::draw_tinted_rotated(SDL_Colour tint, util::Point<float> centre, util::Point<float> dest_position, float angle, int flags)
{
	SDL_Colour colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = tint;
	bool was_batching = get_root()->batching;
	if (was_batching == false) start_batch();
	Vertex_Cache::instance()->cache(colours, centre, internal->offset, size, dest_position, angle, 1.0f, flags);
	if (was_batching == false) end_batch();
}

void Image::draw_tinted_rotated_scaledxy_z(SDL_Colour tint, util::Point<float> centre, util::Point<float> dest_position, float angle, float scale_x, float scale_y, float z, int flags)
{
	SDL_Colour colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = tint;
	bool was_batching = get_root()->batching;
	if (was_batching == false) start_batch();
	Vertex_Cache::instance()->cache_z(colours, centre, internal->offset, size, dest_position, angle, scale_x, scale_y, z, flags);
	if (was_batching == false) end_batch();
}

void Image::draw_tinted_rotated_scaled_z(SDL_Colour tint, util::Point<float> centre, util::Point<float> dest_position, float angle, float scale, float z, int flags)
{
	draw_tinted_rotated_scaledxy_z(tint, centre, dest_position, angle, scale, scale, z, flags);
}

void Image::draw_rotated_scaled_z(util::Point<float> centre, util::Point<float> dest_position, float angle, float scale, float z, int flags)
{
	draw_tinted_rotated_scaledxy_z(shim::white, centre, dest_position, angle, scale, scale, z, flags);
}

void Image::draw_tinted_rotated_scaled(SDL_Colour tint, util::Point<float> centre, util::Point<float> dest_position, float angle, float scale, int flags)
{
	draw_tinted_rotated_scaled_z(tint, centre, dest_position, angle, scale, 0.0f, flags);
}

void Image::draw_tinted_rotated_scaledxy(SDL_Colour tint, util::Point<float> centre, util::Point<float> dest_position, float angle, float scale_x, float scale_y, int flags)
{
	draw_tinted_rotated_scaledxy_z(tint, centre, dest_position, angle, scale_x, scale_y, 0.0f, flags);
}

void Image::draw_rotated(util::Point<float> centre, util::Point<float> dest_position, float angle, int flags)
{
	draw_tinted_rotated(shim::white, centre, dest_position, angle, flags);
}

void Image::draw_rotated_scaled(util::Point<float> centre, util::Point<float> dest_position, float angle, float scale, int flags)
{
	draw_tinted_rotated_scaled(shim::white, centre, dest_position, angle, scale, flags);
}

//--

Image::Internal::Internal(std::string filename, bool keep_data, bool support_render_to_texture, bool load_from_filesystem) :
	loaded_data(0),
	filename(filename),
	refcount(1),
	has_render_to_texture(support_render_to_texture),
#ifdef _WIN32
	video_texture(0),
	render_target(0),
#endif
	texture(0),
	depth_buffer(0),
	stencil_buffer(0),
	parent(0),
	offset(0, 0),
	opaque_topleft(-1, -1),
	opaque_bottomright(-1, -1)
{
	this->create_depth_buffer = Image::create_depth_buffer;
	this->create_stencil_buffer = Image::create_stencil_buffer;
	loaded_data = reload(keep_data, load_from_filesystem);
}

Image::Internal::Internal(unsigned char *pixels, util::Size<int> size, bool support_render_to_texture) :
	loaded_data(0),
	size(size),
	has_render_to_texture(support_render_to_texture),
#ifdef _WIN32
	video_texture(0),
	render_target(0),
#endif
	texture(0),
	depth_buffer(0),
	stencil_buffer(0),
	parent(0),
	offset(0, 0),
	opaque_topleft(-1, -1),
	opaque_bottomright(-1, -1)
{
	this->create_depth_buffer = Image::create_depth_buffer;
	this->create_stencil_buffer = Image::create_stencil_buffer;
	filename = "--NOT LOADED--";
	upload(pixels);
}

Image::Internal::Internal() :
	loaded_data(0),
	has_render_to_texture(false),
#ifdef _WIN32
	video_texture(0),
	render_target(0),
#endif
	texture(0),
	depth_buffer(0),
	stencil_buffer(0),
	parent(0),
	offset(0, 0),
	opaque_topleft(-1, -1),
	opaque_bottomright(-1, -1)
{
	this->create_depth_buffer = Image::create_depth_buffer;
	this->create_stencil_buffer = Image::create_stencil_buffer;
}

Image::Internal::~Internal()
{
	release();

	delete[] loaded_data;
	loaded_data = 0;
}

void Image::Internal::release()
{
	if (parent) {
		return;
	}

	unbind();

	if (shim::opengl) {
		if (bound_fbo == fbo) {
			bound_fbo = 0;
		}
		if (has_render_to_texture) {
			if (depth_buffer != 0) {
				glDeleteRenderbuffers_ptr(1, &depth_buffer);
				depth_buffer = 0;
			}
			if (stencil_buffer != 0) {
				glDeleteRenderbuffers_ptr(1, &stencil_buffer);
				stencil_buffer = 0;
			}
			if (fbo != 0) {
				glDeleteFramebuffers_ptr(1, &fbo);
				fbo = 0;
			}
		}

		if (texture != 0) {
			glDeleteTextures_ptr(1, &texture);
			PRINT_GL_ERROR("glDeleteTextures\n");
			texture = 0;
		}
	}
#ifdef _WIN32
	else {
		if (video_texture) {
			util::verbosemsg("Internal::release (%p), video_texture->Release=%d\n", this, video_texture->Release());
			video_texture = 0;
			d3d_video_texture_count--;
		}

		if (has_render_to_texture) {
			if (system_texture) {
				util::verbosemsg("Internal::release (%p), system_texture->Release=%d\n", this, system_texture->Release());
				system_texture = 0;
				d3d_system_texture_count--;
			}

			if (depth_stencil_buffer != 0) {
				if (bound_depth_buffer == depth_stencil_buffer) {
					shim::d3d_device->SetDepthStencilSurface(0);
				}

				bound_depth_buffer = 0;
				depth_buffer_bound = true;

				util::verbosemsg("Internal::release(%p), depth_stencil_buffer->Release=%d\n", this, depth_stencil_buffer->Release());
				depth_stencil_buffer = 0;
				
				d3d_depth_buffer_count--;
			}
		}
		
		if (render_target) {
			util::verbosemsg("Image::Internal::release (%p), render_target->release=%d\n", this, render_target->Release());
			//while (render_target->Release());
			render_target = 0;
			d3d_surface_level_count--;
		}
	}
#endif
}

unsigned char *Image::Internal::reload(bool keep_data, bool load_from_filesystem)
{
	std::string ext;

	if (filename.length() < 3) {
		ext = "tga";
	}
	else {
		ext = filename.substr(filename.length()-3);
	}

	ext = util::uppercase(ext);

	unsigned char *pixels;

	if (ext == "TGA") {
		pixels = Image::read_tga(filename, size, NULL, &opaque_topleft, &opaque_bottomright, &has_alpha, load_from_filesystem);
	}
#ifdef USE_PNG
	else {
		pixels = Image::read_png(filename, size, NULL, &opaque_topleft, &opaque_bottomright, &has_alpha, load_from_filesystem);
	}
#endif

	if (pixels == 0) {
		return 0;
	}

	try {
		upload(pixels);
	}
	catch (util::Error &) {
		delete[] pixels;
		throw;
	}

	if (keep_data == false) {
		delete[] pixels;
		return 0;
	}
	else {
		return pixels;
	}
}

void Image::Internal::upload(unsigned char *pixels)
{
	// To get a complete palette..
	if (dumping_colours) {
		unsigned char *rgb = pixels;
		for (int i = 0; i < size.w*size.h; i++) {
			if (rgb[3] != 0) {
				printf("rgb: %d %d %d\n", rgb[0], rgb[1], rgb[2]);
			}
			rgb += 4;
		}
	}

	if (shim::opengl && texture == 0) {
		glGenTextures_ptr(1, &texture);
		PRINT_GL_ERROR("glGenTextures\n");
		if (texture == 0) {
			throw util::GLError("glGenTextures failed");
		}

		glActiveTexture_ptr(GL_TEXTURE0);
		PRINT_GL_ERROR("glActiveTexture\n");

		glBindTexture_ptr(GL_TEXTURE_2D, texture);
		PRINT_GL_ERROR("glBindTexture\n");

#if 0
		glTexImage2D_ptr(GL_TEXTURE_2D, 0, GL_RGBA4, size.w, size.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#else
		glTexImage2D_ptr(GL_TEXTURE_2D, 0, GL_RGBA, size.w, size.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#endif
		PRINT_GL_ERROR("glTexImage2D\n");

		glTexParameteri_ptr(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		PRINT_GL_ERROR("glTexParameteri\n");
		glTexParameteri_ptr(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		PRINT_GL_ERROR("glTexParameteri\n");
		glTexParameteri_ptr(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		PRINT_GL_ERROR("glTexParameteri\n");
		glTexParameteri_ptr(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		PRINT_GL_ERROR("glTexParameteri\n");

		if (has_render_to_texture) {
			// Create an FBO for render-to-texture

			glGenFramebuffers_ptr(1, &fbo);
			PRINT_GL_ERROR("glGenFramebuffers\n");

			glBindFramebuffer_ptr(GL_FRAMEBUFFER, fbo);
			PRINT_GL_ERROR("glBindFramebuffer\n");

			glFramebufferTexture2D_ptr(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
			PRINT_GL_ERROR("glFramebufferTexture2D\n");

			if (this->create_depth_buffer) {
				GLenum format;
				if (this->create_stencil_buffer) {
					format = GL_DEPTH24_STENCIL8;
				}
				else {
					format = GL_DEPTH_COMPONENT16;
				}
#if defined ANDROID || defined IOS || defined RASPBERRYPI
				if (strstr((const char *)glGetString(GL_EXTENSIONS), "GL_OES_packed_depth_stencil") != 0) {
					glGenRenderbuffers_ptr(1, &depth_buffer); // use a combined depth and stencil as it must be supported
					PRINT_GL_ERROR("glGenRenderbuffers\n");
					glBindRenderbuffer_ptr(GL_RENDERBUFFER, depth_buffer);
					PRINT_GL_ERROR("glBindRenderbuffer\n");
					glRenderbufferStorage_ptr(GL_RENDERBUFFER, format, size.w, size.h);
					PRINT_GL_ERROR("glRenderbufferStorage\n");
					glFramebufferRenderbuffer_ptr(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
					PRINT_GL_ERROR("glFramebufferRenderbuffer\n");
					if (this->create_stencil_buffer) {
						glFramebufferRenderbuffer_ptr(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
						PRINT_GL_ERROR("glFramebufferRenderbuffer\n");
					}
				}
				else { // there will be no stencil buffer
					glGenRenderbuffers_ptr(1, &depth_buffer);
					PRINT_GL_ERROR("glGenRenderbuffers\n");
					glBindRenderbuffer_ptr(GL_RENDERBUFFER, depth_buffer);
					PRINT_GL_ERROR("glBindRenderbuffer\n");
					glRenderbufferStorage_ptr(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, size.w, size.h);
					PRINT_GL_ERROR("glRenderbufferStorage\n");
					glFramebufferRenderbuffer_ptr(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
					PRINT_GL_ERROR("glFramebufferRenderbuffer\n");
				}
#else
				glGenRenderbuffers_ptr(1, &depth_buffer); // use a combined depth and stencil as it must be supported
				PRINT_GL_ERROR("glGenRenderbuffers\n");
				glBindRenderbuffer_ptr(GL_RENDERBUFFER, depth_buffer);
				PRINT_GL_ERROR("glBindRenderbuffer\n");
				glRenderbufferStorage_ptr(GL_RENDERBUFFER, format, size.w, size.h);
				PRINT_GL_ERROR("glRenderbufferStorage\n");
				glFramebufferRenderbuffer_ptr(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
				PRINT_GL_ERROR("glFramebufferRenderbuffer\n");
				if (this->create_stencil_buffer) {
					glFramebufferRenderbuffer_ptr(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
					PRINT_GL_ERROR("glFramebufferRenderbuffer\n");
				}
#endif
			}

			GLenum result = glCheckFramebufferStatus_ptr(GL_FRAMEBUFFER);
			if (result != GL_FRAMEBUFFER_COMPLETE) {
				throw util::GLError("Incomplete framebuffer!");
			}
			PRINT_GL_ERROR("glCheckFramebufferStatus\n");

#ifdef IOS
			if (bound_fbo == 0) {
				glBindRenderbuffer_ptr(GL_RENDERBUFFER, internal::gfx_context.colorbuffer);
				glBindFramebuffer_ptr(GL_FRAMEBUFFER, internal::gfx_context.framebuffer);
			}
			else {
#endif
				glBindFramebuffer_ptr(GL_FRAMEBUFFER, bound_fbo);
#ifdef IOS
			}
#endif

			PRINT_GL_ERROR("glBindFramebuffer\n");
		}

		Shader::rebind_opengl_texture0();
	}
#ifdef _WIN32
	else if (video_texture == 0) {
		int err;

		if (has_render_to_texture) {
			err = shim::d3d_device->CreateTexture(size.w, size.h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &video_texture, 0);
			if (err != D3D_OK) {
				util::errormsg("CreateTexture failed for video texture (%dx%d, %d).\n", size.w, size.h, err);
			}
			d3d_video_texture_count++;

			err = shim::d3d_device->CreateTexture(size.w, size.h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &system_texture, 0);
			if (err != D3D_OK) {
				util::errormsg("CreateTexture failed for system texture (%dx%d, %d).\n", size.w, size.h, err);
			}
			d3d_system_texture_count++;

			D3DLOCKED_RECT locked_rect;
			if (system_texture->LockRect(0, &locked_rect, 0, 0) == D3D_OK) {
				for (int y = 0; y < size.h; y++) {
					unsigned char *dest = ((unsigned char *)locked_rect.pBits) + y * locked_rect.Pitch;
					for (int x = 0; x < size.w; x++) {
						unsigned char r = *pixels++;
						unsigned char g = *pixels++;
						unsigned char b = *pixels++;
						unsigned char a = *pixels++;
						*dest++ = b;
						*dest++ = g;
						*dest++ = r;
						*dest++ = a;
					}
				}
				system_texture->UnlockRect(0);
			}
			else {
				util::errormsg("Unable to lock system texture.\n");
			}

			if (shim::d3d_device->UpdateTexture((IDirect3DBaseTexture9 *)system_texture, (IDirect3DBaseTexture9 *)video_texture) != D3D_OK) {
				util::errormsg("UpdateTexture failed.\n");
			}

			if (this->create_depth_buffer) {
				D3DFORMAT format;
				if (this->create_stencil_buffer) {
					format = D3DFMT_D24S8;
				}
				else {
					format = D3DFMT_D16;
				}

				// Direct3D9 can't render if the depth buffer is smaller than the largest texture or screen: so adjust the size of the depth buffer if needed
				util::Size<int> depth_buffer_size;
				if (size.w < shim::real_screen_size.w || size.h < shim::real_screen_size.h) {
					depth_buffer_size = shim::real_screen_size;
				}
				else {
					depth_buffer_size = size;
				}
				if (shim::d3d_device->CreateDepthStencilSurface(depth_buffer_size.w, depth_buffer_size.h, format, D3DMULTISAMPLE_NONE, 0, true, &depth_stencil_buffer, 0) != D3D_OK) {
					throw util::Error("CreateDepthStencilSurface failed");
				}

				d3d_depth_buffer_count++;

				shim::d3d_device->SetDepthStencilSurface(depth_stencil_buffer);

				shim::d3d_device->Clear(0, 0, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

				if (depth_buffer_bound) {
					shim::d3d_device->SetDepthStencilSurface(bound_depth_buffer);
				}
				else {
					shim::d3d_device->SetDepthStencilSurface(internal::gfx_context.depth_stencil_buffer);
				}
				depth_buffer_bound = true;
			}
			else {
				depth_stencil_buffer = 0;
			}
		}
		else {
			err = shim::d3d_device->CreateTexture(size.w, size.h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &video_texture, 0);
			if (err != D3D_OK) {
				util::errormsg("CreateTexture failed for video texture (%dx%d, %d).\n", size.w, size.h, err);
			}
			d3d_video_texture_count++;

			D3DLOCKED_RECT locked_rect;
			if (video_texture->LockRect(0, &locked_rect, 0, 0) == D3D_OK) {
				for (int y = 0; y < size.h; y++) {
					unsigned char *dest = ((unsigned char *)locked_rect.pBits) + y * locked_rect.Pitch;
					for (int x = 0; x < size.w; x++) {
						unsigned char r = *pixels++;
						unsigned char g = *pixels++;
						unsigned char b = *pixels++;
						unsigned char a = *pixels++;
						*dest++ = b;
						*dest++ = g;
						*dest++ = r;
						*dest++ = a;
					}
				}
				video_texture->UnlockRect(0);
			}
			else {
				util::errormsg("Unable to lock video texture.\n");
			}

			depth_stencil_buffer = 0;
		}
	}
#endif
}

void Image::Internal::unbind()
{
	std::vector< std::pair<std::string, Image *> > &bound_images = Shader::get_bound_images();
	for (size_t unit = 0; unit < bound_images.size(); unit++) {
		std::pair<std::string, Image *> p = bound_images[unit];
		if (p.second->internal == this) {
			shim::current_shader->set_texture(p.first, 0, (int)unit);
			bound_images[unit].second = nullptr;
			break;
		}
	}
}

void Image::Internal::destroy_data()
{
	delete[] loaded_data;
	loaded_data = 0;
}

bool Image::Internal::is_transparent(util::Point<int> position)
{
	if (loaded_data == 0) {
		return false;
	}

	unsigned char *p = loaded_data + ((((size.h-1)-position.y) * size.w) + position.x) * 4;

	return p[3] == 0;
}

} // End namespace gfx

} // End namespace noo
