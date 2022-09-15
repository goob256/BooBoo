#ifdef USE_TTF

#include "shim4/error.h"
#include "shim4/gfx.h"
#include "shim4/image.h"
#include "shim4/shader.h"
#include "shim4/shim.h"
#include "shim4/ttf.h"
#include "shim4/utf8.h"
#include "shim4/util.h"

#include "shim4/internal/gfx.h"

using namespace noo;

namespace noo {

namespace gfx {

TTF::TTF(std::string filename, int size, int sheet_size) :
	sheet_size(sheet_size)
{
	filename = "gfx/fonts/" + filename;

	int sz;

	file = util::open_file(filename, &sz); // FIXME: does this get automatically closed? (I think yes)

	font = TTF_OpenFontRW(file, true, size*0.75f); // 1 px == 0.75 pt

	if (font == 0) {
		throw util::LoadError("TTF_OpenFontRW failed");
	}

	// FIXME?
	//TTF_SetFontHinting(font, TTF_HINTING_NONE);
	TTF_SetFontHinting(font, TTF_HINTING_NORMAL);

	smooth = true;

	num_glyphs = 0;

	this->size = TTF_FontHeight(font);

	curr_sheet = 0;
}

TTF::~TTF()
{
	clear_cache();

	for (std::vector<Font *>::iterator it = loaded_fonts.begin(); it != loaded_fonts.end(); it++) {
		Font *f = *it;
		if (this == f) {
			loaded_fonts.erase(it);
			break;
		}
	}

	TTF_CloseFont(font);

	util::free_data(file);
}

void TTF::clear_cache()
{
	std::map<Uint32, Glyph *>::iterator it;
	for  (it = glyphs.begin(); it != glyphs.end(); it++) {
		std::pair<int, Glyph *> p = *it;
		delete p.second;
	}
	glyphs.clear();
	num_glyphs = 0;

	for (size_t i = 0; i < sheets.size(); i++) {
		delete sheets[i];
	}
	sheets.clear();

	curr_sheet = 0;
}

int TTF::get_height()
{
	return size;
}

int TTF::get_sheet_size()
{
	return sheet_size;
}

void TTF::set_sheet_target(int sheet_num)
{
	bool created;

	if (sheets.size() <= sheet_num) {
		while (sheets.size() <= sheet_num) {
			if (std::find(loaded_fonts.begin(), loaded_fonts.end(), this) == loaded_fonts.end()) {
				loaded_fonts.push_back(this);
			}

			int this_sheet_size = get_sheet_size();

			bool cdb = Image::create_depth_buffer;
			Image::create_depth_buffer = false;
			gfx::Image *sheet = new Image(util::Size<int>(this_sheet_size, this_sheet_size)); // FIXME: adjust size for other languages
			Image::create_depth_buffer = cdb;

			sheets.push_back(sheet);
		}
		created = true;
	}
	else {
		created = false;
	}

	old_target = get_target_image();
	gfx::get_matrices(modelview, proj);
	old_shader = shim::current_shader;

	shim::current_shader = shim::default_shader;
	shim::current_shader->use();
	set_target_image(sheets[sheet_num]);

	if (created) {
		gfx::clear(shim::transparent);
	}
}

void TTF::restore_target()
{
	shim::current_shader = old_shader;
	shim::current_shader->use();
	set_target_image(old_target);
	gfx::set_matrices(modelview, proj);
	gfx::update_projection();
}

TTF::Glyph *TTF::create_glyph(Uint32 ch, gfx::Image *glyph_image)
{
	int this_sheet_size = get_sheet_size();

	int n = num_glyphs;
	int n_per_row = this_sheet_size / (size + 2);
	int row = n / n_per_row;
	int col = n % n_per_row;
	Glyph *glyph = new Glyph;
	glyph->position = {col * (size+2) + 1, row * (size+2) + 1};
	glyph->size = glyph_image->size;
	glyph->sheet = curr_sheet;

	glyphs[ch] = glyph;

	num_glyphs++;

	return glyph;
}

void TTF::render_glyph(Glyph *glyph, gfx::Image *glyph_image)
{
	glyph_image->start_batch();
	glyph_image->draw(glyph->position, Image::FLIP_V); // glyphs are rendered upside down
	glyph_image->end_batch();
}

gfx::Image *TTF::load_glyph_image(Uint32 ch)
{
	SDL_Surface *surface;

	if (smooth) {
		surface = TTF_RenderGlyph_Blended(font, (Uint16)ch, shim::white);
	}
	else {
		surface = TTF_RenderGlyph_Solid(font, (Uint16)ch, shim::white);
	}

	if (surface == 0) {
		util::errormsg("Error rendering glyph.\n");
		return 0;
	}

	bool cdb = Image::create_depth_buffer;
	Image::create_depth_buffer = false;
	Image *glyph_image = new Image(surface);
	Image::create_depth_buffer = cdb;
	
	SDL_FreeSurface(surface);
	
	return glyph_image;
}

bool TTF::cache_glyphs(std::string text)
{
#ifdef _WIN32
	if (internal::gfx_context.d3d_lost) {
		return false;
	}
#endif

	int offset = 0;
	Uint32 ch;
	while ((ch = util::utf8_char_next(text, offset)) != 0) {
		if (glyphs.find(ch) == glyphs.end()) {
			int sheet_size = get_sheet_size();
			int n = num_glyphs;
			int n_per_row = sheet_size / (size + 2);
			if (num_glyphs >= n_per_row*n_per_row) {
				curr_sheet++;
				num_glyphs = 0;
			}
			set_sheet_target(curr_sheet);
			gfx::Image *glyph_image = load_glyph_image(ch);
			if (glyph_image == 0) {
				gfx::set_target_backbuffer();
				return false;
			}
			Glyph *glyph = create_glyph(ch, glyph_image);
			if (glyph == 0) {
				gfx::set_target_backbuffer();
				delete glyph_image;
				return false;
			}
			render_glyph(glyph,  glyph_image);
			delete glyph_image;
		}
	}

	gfx::set_target_backbuffer();

	return true;
}

void TTF::set_smooth(bool smooth)
{
	this->smooth = smooth;

	if (smooth) {
		TTF_SetFontHinting(font, TTF_HINTING_NORMAL);
	}
	else {
		TTF_SetFontHinting(font, TTF_HINTING_NONE);
	}
}

} // End namespace gfx

} // End namespace noo

#endif // USE_TTF
