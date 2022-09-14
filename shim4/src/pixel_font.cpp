#include "shim4/pixel_font.h"
#include "shim4/image.h"
#include "shim4/tokenizer.h"
#include "shim4/utf8.h"
#include "shim4/util.h"

#define PAD 2

using namespace noo;

namespace noo {

namespace gfx {

Pixel_Font::Pixel_Font(std::string name)
{
	std::string dirname = "gfx/fonts/" + name + "/";

	bool old_keep_data = gfx::Image::keep_data;
	gfx::Image::keep_data = true;
	gfx::Image *sheet = new gfx::Image(dirname + "font.tga", true);
	sheets.push_back(sheet);
	gfx::Image::keep_data = old_keep_data;

	std::string info = util::load_text(dirname + "info.ini");

	util::Tokenizer t(info, '\n');
	std::string line;

	size = 10; // default

	bool wide = false;

	while ((line = t.next()) != "") {
		util::trim(line);
		util::Tokenizer t2(line, '=');
		std::string key = t2.next();
		std::string value = t2.next();

		util::trim(key);
		util::trim(value);

		if (key == "size") {
			size = atoi(value.c_str());
		}
		else if (key == "wide") {
			wide = atoi(value.c_str());
		}
	}

	int wmul = wide ? 2 : 1;
	
	size_w = size * wmul;

	int glyphs_wide = sheets[0]->size.w / ((size_w + PAD*2)*3);

	unsigned char *loaded_data = sheets[0]->get_loaded_data();

	std::string g = util::load_text(dirname + "glyphs.utf8");
	Uint32 ch;
	int offset = 0;
	int count = 0;
	while ((ch = util::utf8_char_next(g, offset)) != 0) {
		int x = count % glyphs_wide;
		int y = count / glyphs_wide;
		Glyph *glyph = new Glyph;
		glyph->position = {x * ((size_w + PAD*2)*3) + PAD, y * (size + PAD*2) + PAD};
		glyph->size.h = size;
		int max_x;
		if (ch == ' ') {
			max_x = 1 * (wmul == 1 ? 1 : 3); // FIXME: need a better way
		}
		else {
			max_x = 0;
			for (int y = 0; y < size; y++) {
				unsigned char *p = loaded_data + (sheets[0]->size.w * 4 * (sheets[0]->size.h-1)) - ((glyph->position.y+y) * sheets[0]->size.w * 4) + (glyph->position.x * 4);
				for (int x = 0; x < size_w; x++) {
					if (x > max_x && p[3] != 0) {
						max_x = x;
					}
					p += 4;
				}
			}
		}
		glyph->size.w = max_x + 2; // +1 for an extra pixel space and +1 because max_x is an index and this is a size
		glyphs[ch] = glyph;
		count++;
	}
}

Pixel_Font::~Pixel_Font()
{
	for (std::vector<Font *>::iterator it = loaded_fonts.begin(); it != loaded_fonts.end(); it++) {
		Font *f = *it;
		if (this == f) {
			loaded_fonts.erase(it);
			break;
		}
	}

	std::map<Uint32, Glyph *>::iterator it;
	for  (it = glyphs.begin(); it != glyphs.end(); it++) {
		std::pair<int, Glyph *> p = *it;
		delete p.second;
	}

	gfx::Image *sheet = sheets[0];
	delete sheet;
	sheets.clear();
}

void Pixel_Font::clear_cache()
{
}

int Pixel_Font::get_height()
{
	return size;
}

bool Pixel_Font::cache_glyphs(std::string text)
{
	// This can safely be here
	if (std::find(loaded_fonts.begin(), loaded_fonts.end(), this) == loaded_fonts.end()) {
		loaded_fonts.push_back(this);
	}
	return true;
}

} // End namespace gfx

} // End namespace noo
