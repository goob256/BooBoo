#ifndef NOO_IMAGE_H
#define NOO_IMAGE_H

#include "shim4/main.h"

namespace noo {

namespace gfx {

class Shader;

class Image {
public:
	friend class Shader;
	friend class Vertex_Cache;

	enum Flags {
		FLIP_H = 1,
		FLIP_V = 2
	};

	static void static_start();
	static void release_all(bool include_managed = false);
	static void reload_all(bool include_managed = false);
	static int get_unfreed_count();
	static void audit();
	static unsigned char *read_png(std::string filename, util::Size<int> &out_size, SDL_Colour *out_palette = 0, util::Point<int> *opaque_topleft = 0, util::Point<int> *opaque_bottomright = 0, bool *has_alpha = 0, bool load_from_filesystem = false);
	static unsigned char *read_tga(std::string filename, util::Size<int> &out_size, SDL_Colour *out_palette = 0, util::Point<int> *opaque_topleft = 0, util::Point<int> *opaque_bottomright = 0, bool *has_alpha = 0, bool load_from_filesystem = false);
	static unsigned char *read_texture(gfx::Image *image);

	// These parameters affect newly created images
	static bool dumping_colours;
	static bool keep_data;
	static bool save_rle;
	static bool ignore_palette;
	static bool create_depth_buffer;
	static bool create_stencil_buffer;
	static bool premultiply_alpha;
	static bool save_rgba;
	static bool save_palettes;

	std::string filename;
	util::Size<int> size;

	SHIM4_EXPORT Image(std::string filename, bool is_absolute_path = false, bool load_from_filesystem = false);
	SHIM4_EXPORT Image(Uint8 *data, util::Size<int> size, bool destroy_data = false);
	SHIM4_EXPORT Image(SDL_Surface *surface);
	SHIM4_EXPORT Image(util::Size<int> size);
	SHIM4_EXPORT Image(util::Size<int> size, unsigned char *pixels);
	SHIM4_EXPORT Image(Image *parent, util::Point<int> offset, util::Size<int> size);
	SHIM4_EXPORT virtual ~Image();

	SHIM4_EXPORT void release();
	SHIM4_EXPORT void reload(bool load_from_filesystem = false);

	SHIM4_EXPORT bool save(std::string filename);

	// these are not normally to be used, use gfx::set_target_image and gfx::set_target_backbuffer instead
	SHIM4_EXPORT void set_target();
	SHIM4_EXPORT void release_target();

	SHIM4_EXPORT void get_bounds(util::Point<int> &topleft, util::Point<int> &bottomright);
	SHIM4_EXPORT void set_bounds(util::Point<int> topleft, util::Point<int> bottomright);

	SHIM4_EXPORT void destroy_data();

	SHIM4_EXPORT Image *get_root();

	SHIM4_EXPORT unsigned char *get_loaded_data();

	SHIM4_EXPORT void start_batch(bool repeat = false);
	SHIM4_EXPORT void end_batch();

	SHIM4_EXPORT void stretch_region_tinted_repeat(SDL_Colour tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, util::Size<int> dest_size, int flags = 0);
	SHIM4_EXPORT void stretch_region_tinted(SDL_Colour tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, util::Size<int> dest_size, int flags = 0);
	SHIM4_EXPORT void stretch_region(util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, util::Size<int> dest_size, int flags = 0);
	SHIM4_EXPORT void draw_region_lit_z_range(SDL_Colour colours[4], util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z_top, float z_bottom, int flags = 0);
	SHIM4_EXPORT void draw_region_lit_z(SDL_Colour colours[4], util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z, int flags = 0);
	SHIM4_EXPORT void draw_region_tinted_z_range(SDL_Colour tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z_top, float z_bottom, int flags = 0);
	SHIM4_EXPORT void draw_region_tinted_z(SDL_Colour tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z, int flags = 0);
	SHIM4_EXPORT void draw_region_tinted(SDL_Colour tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, int flags = 0);
	SHIM4_EXPORT void draw_region_z_range(util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z_top, float z_bottom, int flags = 0);
	SHIM4_EXPORT void draw_region_z(util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z, int flags = 0);
	SHIM4_EXPORT void draw_region(util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, int flags = 0);
	SHIM4_EXPORT void draw_z(util::Point<float> dest_position, float z, int flags = 0);
	SHIM4_EXPORT void draw_tinted(SDL_Colour tint, util::Point<float> dest_position, int flags = 0);
	SHIM4_EXPORT void draw(util::Point<float> dest_position, int flags = 0);
	SHIM4_EXPORT void draw_tinted_rotated(SDL_Colour tint, util::Point<float> centre, util::Point<float> dest_position, float angle, int flags = 0);
	SHIM4_EXPORT void draw_tinted_rotated_scaledxy_z(SDL_Colour tint, util::Point<float> centre, util::Point<float> dest_position, float angle, float scale_x, float scale_y, float z, int flags = 0);
	SHIM4_EXPORT void draw_tinted_rotated_scaled_z(SDL_Colour tint, util::Point<float> centre, util::Point<float> dest_position, float angle, float scale, float z, int flags = 0);
	SHIM4_EXPORT void draw_rotated_scaled_z(util::Point<float> centre, util::Point<float> dest_position, float angle, float scale, float z, int flags = 0);
	SHIM4_EXPORT void draw_tinted_rotated_scaled(SDL_Colour tint, util::Point<float> centre, util::Point<float> dest_position, float angle, float scale, int flags = 0);
	SHIM4_EXPORT void draw_tinted_rotated_scaledxy(SDL_Colour tint, util::Point<float> centre, util::Point<float> dest_position, float angle, float scale_x, float scale_y, int flags = 0);
	SHIM4_EXPORT void draw_rotated(util::Point<float> centre, util::Point<float> dest_position, float angle, int flags = 0);
	SHIM4_EXPORT void draw_rotated_scaled(util::Point<float> centre, util::Point<float> dest_position, float angle, float scale, int flags = 0);

protected:
	struct TGA_Header {
		char idlength;
		char colourmaptype;
		char datatypecode;
		short int colourmaporigin;
		short int colourmaplength;
		char colourmapdepth;
		short int x_origin;
		short int y_origin;
		short width;
		short height;
		char bitsperpixel;
		char imagedescriptor;
		SDL_Colour palette[256];
	};

	// returns true if pixel is transparent
	static bool merge_bytes(unsigned char *pixel, unsigned char *p, int bytes, TGA_Header *header, bool *alpha);

	SHIM4_EXPORT unsigned char find_colour_in_palette(unsigned char *p);

	SHIM4_EXPORT struct Internal {
		Internal(std::string filename, bool keep_data, bool support_render_to_texture = false, bool load_from_filesystem = false);
		Internal(unsigned char *pixels, util::Size<int> size, bool support_render_to_texture = false);
		Internal();
		~Internal();

		void upload(unsigned char *pixels);

		void release();
		unsigned char *reload(bool keep_data, bool load_from_filesystem = false);
		void unbind();

		void destroy_data();
		bool is_transparent(util::Point<int> position);

		unsigned char *loaded_data;

		std::string filename;
		util::Size<int> size;
		int refcount;

		bool has_render_to_texture;

#ifdef _WIN32
		LPDIRECT3DTEXTURE9 video_texture;
		LPDIRECT3DTEXTURE9 system_texture;
		LPDIRECT3DSURFACE9 render_target;
		LPDIRECT3DSURFACE9 depth_stencil_buffer;
#endif
		GLuint texture;
		GLuint fbo;
		GLuint depth_buffer;
		GLuint stencil_buffer;

		// For sub-images
		Image *parent;
		util::Point<int> offset;

		// Position of outmost opaque pixels
		util::Point<int> opaque_topleft;
		util::Point<int> opaque_bottomright;

		bool create_depth_buffer;
		bool create_stencil_buffer;

		bool has_alpha;
	};

	static std::vector<Internal *> loaded_images;
	static int d3d_depth_buffer_count;
	static int d3d_video_texture_count;
	static int d3d_system_texture_count;
	static int d3d_surface_level_count;

	bool batching;

	Internal *internal;

	bool flipped;

	Uint8 *data_to_destroy;
};

} // End namespace gfx

} // End namespace noo

#endif // NOO_IMAGE_H
