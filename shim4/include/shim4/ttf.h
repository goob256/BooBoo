#ifndef NOO_TTF_H
#define NOO_TTF_H

#include "shim4/main.h"
#include "shim4/font.h"

namespace noo {

namespace gfx {

class Shader;
class Image;

class TTF : public Font
{
public:
	static const int DEFAULT_SHEET_SIZE = 512;

	TTF(std::string filename, int size, int sheet_size = DEFAULT_SHEET_SIZE);
	virtual ~TTF();

	bool cache_glyphs(std::string text);
	void clear_cache();

	int get_height();

	void set_smooth(bool smooth);
	
private:
	int get_sheet_size();
	void set_sheet_target(int sheet_num);
	void restore_target();
	Glyph *create_glyph(Uint32 ch, gfx::Image *glyph_image);
	void render_glyph(Glyph *glyph, gfx::Image *glyph_image);
	gfx::Image *load_glyph_image(Uint32 ch);

	TTF_Font *font;

	int num_glyphs;

	int sheet_size;

	glm::mat4 modelview, proj;
	gfx::Shader *old_shader;
	gfx::Image *old_target;

	SDL_RWops *file;

	int curr_sheet;

	bool smooth;
};

} // End namespace gfx

} // End namespace noo

#endif // NOO_TTF_H
