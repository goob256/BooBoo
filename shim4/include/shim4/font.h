#ifndef NOO_FONT_H
#define NOO_FONT_H

#include "shim4/main.h"

namespace noo {

namespace gfx {

class Image;

ALIGN(16) Font {
public:
	static std::string strip_codes(std::string text, bool strip_colour_codes, bool strip_wrap_codes, bool strip_extra_glyph_codes);
	static void static_start();
	static void release_all();

	SHIM4_EXPORT Font();
	SHIM4_EXPORT virtual ~Font();

	SHIM4_EXPORT virtual bool cache_glyphs(std::string text) = 0;
	SHIM4_EXPORT virtual void clear_cache() = 0;

	SHIM4_EXPORT virtual int get_text_width(std::string text, bool interpret_colour_codes = true, bool interpret_extra_glyphs = true);
	SHIM4_EXPORT virtual int get_height() = 0;

	// If interpret_colour_codes is true, this returns the last colour used. Otherwise, it returns the colour passed in.
	SHIM4_EXPORT SDL_Colour draw(SDL_Colour colour, std::string text, util::Point<float> dest_position, bool interpret_colour_codes = true, bool centre = false, bool interpret_extra_glyphs = true);

	// Returns number of characters drawn, plus whether or not it filled the max in bool &full
	SHIM4_EXPORT int draw_wrapped(SDL_Colour colour, std::string text, util::Point<float> dest_position, int w, int line_height, int max_lines, int elapsed, int delay, bool dry_run, bool &full, int &num_lines, int &width, bool interpret_colour_codes = true, bool centre = false, int first_line_indent = 0, bool interpret_extra_glyphs = true);

	SHIM4_EXPORT void add_extra_glyph(int code, Image *image); // for @00 images
	SHIM4_EXPORT void remove_extra_glyph(int code);
	SHIM4_EXPORT void set_extra_glyph_offset(util::Point<float> offset);

	// For 16 byte alignment to make glm::mat4 able to use SIMD
#ifdef _WIN32
	SHIM4_EXPORT void *operator new(size_t i);
	SHIM4_EXPORT void operator delete(void* p);
#endif
	
protected:
	static std::vector<Font *> loaded_fonts;

	struct Glyph {
		util::Point<int> position;
		util::Size<int> size;
		int sheet;
	};

	std::map<Uint32, Glyph *> glyphs;

	std::vector<Image *> sheets;

	int size_w;
	int size;

	std::map<int, Image *> extra_glyphs;
	util::Point<float> extra_glyph_offset;
};

} // End namespace gfx

} // End namespace noo

#endif // NOO_FONT_H
