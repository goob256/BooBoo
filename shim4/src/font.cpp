#include "shim4/font.h"
#include "shim4/image.h"
#include "shim4/shim.h"
#include "shim4/utf8.h"
#include "shim4/util.h"

using namespace noo;

namespace noo {

namespace gfx {

std::vector<Font *> Font::loaded_fonts;

std::string Font::strip_codes(std::string text, bool strip_colour_codes, bool strip_wrap_codes, bool strip_extra_glyph_codes)
{
	Uint32 ch;
	int offset = 0;
	std::string stripped;
	while ((ch = util::utf8_char_next(text, offset)) != 0) {
		// Colour code interpretation
		if (strip_colour_codes && ch == '|') {
			Uint32 a = util::utf8_char_next(text, offset);
			if (a == 0) {
				break;
			}
			Uint32 b = util::utf8_char_next(text, offset);
			if (b == 0) {
				break;
			}
			continue;
		}
		// Extra glyphs
		if (strip_extra_glyph_codes && ch == '@') {
			Uint32 a = util::utf8_char_next(text, offset);
			if (a == 0) {
				break;
			}
			Uint32 b = util::utf8_char_next(text, offset);
			if (b == 0) {
				break;
			}
			continue;
		}
		else if (strip_wrap_codes && (ch == '^' || ch == '$')) {
			continue;
		}
		stripped += util::utf8_char_to_string(ch);
	}
	return stripped;
}

void Font::static_start()
{
	loaded_fonts.clear();
}

void Font::release_all()
{
	for (std::vector<Font *>::iterator it = loaded_fonts.begin(); it != loaded_fonts.end(); it++) {
		Font *f = *it;
		f->clear_cache();
	}
}

Font::Font()
{
}

Font::~Font()
{
}

int Font::get_text_width(std::string text, bool interpret_colour_codes, bool interpret_extra_glyphs)
{
	if (cache_glyphs(strip_codes(text, interpret_colour_codes, false, interpret_extra_glyphs)) == false) {
		return 0;
	}

	int width = 0;
	int offset = 0;
	int ch;

	while ((ch = util::utf8_char_next(text, offset)) != 0) {
		// Colour code interpretation
		if (interpret_colour_codes && ch == '|') {
			// Skip them here, use them below
			int a = util::utf8_char_next(text, offset);
			if (a == 0) {
				break;
			}
			int b = util::utf8_char_next(text, offset);
			if (b == 0) {
				break;
			}

			continue;
		}
		// Extra glyphs
		if (interpret_extra_glyphs && ch == '@') {
			// Skip them here, use them below
			int a = util::utf8_char_next(text, offset);
			if (a == 0) {
				break;
			}
			int b = util::utf8_char_next(text, offset);
			if (b == 0) {
				break;
			}

			if (a >= 'A' && a <= 'F') {
				a = a - 'A' + 10;
			}
			else if (a >= 'a' && a <= 'f') {
				a = a - 'a' + 10;
			}
			else {
				a = a - '0';
			}
			if (b >= 'A' && b <= 'F') {
				b = b - 'A' + 10;
			}
			else if (b >= 'a' && b <= 'f') {
				b = b - 'a' + 10;
			}
			else {
				b = b - '0';
			}
			
			int extra_glyph = a * 16 + b;

			std::map<int, Image *>::iterator it = extra_glyphs.find(extra_glyph);
			if (it == extra_glyphs.end()) {
				width += 5;
			}
			else {
				std::pair<int, Image *> p = *it;
				width += p.second->size.w + 1;
			}
			continue;
		}

		std::map<Uint32, Glyph *>::iterator it = glyphs.find(ch);

		if (it == glyphs.end()) {
			width += 5;
			continue;
		}

		std::pair<Uint32, Glyph *> pair = *it;
		Glyph *glyph = pair.second;
		width += glyph->size.w;
	}

	return width - 1; // glyph size includes advance, subtract 1 because no advance is needed on the final character
}

SDL_Colour Font::draw(SDL_Colour colour, std::string text, util::Point<float> dest_position, bool interpret_colour_codes, bool centre, bool interpret_extra_glyphs)
{
	if (text == "") {
		return colour;
	}

	if (cache_glyphs(strip_codes(text, interpret_colour_codes, false, interpret_extra_glyphs)) == false) {
		return colour;
	}

	if (centre) {
		dest_position.x -= get_text_width(text, interpret_colour_codes) / 2;
	}

	dest_position.x = dest_position.x;
	dest_position.y = (dest_position.y-1);

	for (size_t sheet = 0; sheet < sheets.size(); sheet++) {
		util::Point<float> pos = dest_position;
		int offset = 0;
		int ch;

		sheets[sheet]->start_batch();

		util::Point<int> glyph_offset;
		util::Size<int> glyph_size_diff;
		util::Point<int> draw_offset;
		int w = size_w + 2;
		glyph_offset = {-1, -1};
		glyph_size_diff = {2, 2};
		draw_offset = {-1, -1};

		while ((ch = util::utf8_char_next(text, offset)) != 0) {
			// Colour code interpretation
			if (interpret_colour_codes && ch == '|') {
				// Skip them here, use them below
				int a = util::utf8_char_next(text, offset);
				if (a == 0) {
					break;
				}
				int b = util::utf8_char_next(text, offset);
				if (b == 0) {
					break;
				}

				if (a >= 'A' && a <= 'F') {
					a = a - 'A' + 10;
				}
				else if (a >= 'a' && a <= 'f') {
					a = a - 'a' + 10;
				}
				else {
					a = a - '0';
				}
				if (b >= 'A' && b <= 'F') {
					b = b - 'A' + 10;
				}
				else if (b >= 'a' && b <= 'f') {
					b = b - 'a' + 10;
				}
				else {
					b = b - '0';
				}
				int index = a * 16 + b;
				colour = shim::palette[index];

				continue;
			}
			// extra glyphs
			if (ch == '@') {
				// Skip them here, use them below
				int a = util::utf8_char_next(text, offset);
				if (a == 0) {
					break;
				}
				int b = util::utf8_char_next(text, offset);
				if (b == 0) {
					break;
				}

				if (a >= 'A' && a <= 'F') {
					a = a - 'A' + 10;
				}
				else if (a >= 'a' && a <= 'f') {
					a = a - 'a' + 10;
				}
				else {
					a = a - '0';
				}
				if (b >= 'A' && b <= 'F') {
					b = b - 'A' + 10;
				}
				else if (b >= 'a' && b <= 'f') {
					b = b - 'a' + 10;
				}
				else {
					b = b - '0';
				}
				
				int extra_glyph = a * 16 + b;

				std::map<int, Image *>::iterator it = extra_glyphs.find(extra_glyph);
				if (it == extra_glyphs.end()) {
					pos.x += 5;
				}
				else {
					sheets[sheet]->end_batch();
					std::pair<int, Image *> p = *it;
					p.second->start_batch();
					p.second->draw_tinted(colour, pos+extra_glyph_offset);
					p.second->end_batch();
					pos.x += p.second->size.w + 1;
					sheets[sheet]->start_batch();
				}
				
				continue;
			}

			std::map<Uint32, Glyph *>::iterator it = glyphs.find(ch);
			if (it == glyphs.end()) {
				if (ch != 160) { // non-breaking space
					util::debugmsg("missing glyph! %u\n", ch);
				}
				continue;
			}

			std::pair<Uint32, Glyph *> pair = *it;
			Glyph *glyph = pair.second;

			if (glyph->sheet == sheet) {
				sheets[sheet]->draw_region_tinted(colour, glyph->position, glyph->size, pos, 0);
			}

			pos.x += glyph->size.w;
			
			if (glyph->sheet != sheet) {
				continue;
			}

		}
		sheets[sheet]->end_batch();
	}

	return colour;
}

int Font::draw_wrapped(SDL_Colour colour, std::string text, util::Point<float> dest_position, int w, int line_height, int max_lines, int elapsed, int delay, bool dry_run, bool &full, int &num_lines, int &width, bool interpret_colour_codes, bool centre, int first_line_indent, bool interpret_extra_glyphs)
{
	if (cache_glyphs(strip_codes(text, interpret_colour_codes, true, interpret_extra_glyphs)) == false) {
		return 0;
	}

	full = false;
	float curr_y = dest_position.y;
	bool done = false;
	int lines = 0;
	if (max_lines == -1) {
		max_lines = 1000000;
	}
	if (elapsed < 0) {
		elapsed = 1000000;
	}
	int chars_to_draw;
	if (delay == 0) {
		chars_to_draw = 1000000;
	}
	else {
		chars_to_draw = elapsed / delay;
		// Must loop through and add for any colour codes/extra glyphs
		for (int i = 0; i < chars_to_draw && text[i] != 0; i++) {
			if ((interpret_colour_codes && text[i] == '|') || (interpret_extra_glyphs && text[i] == '@')) {
				chars_to_draw += text[i] == '|' ? 3 : 2; // skip only 2 for @ because it's an actual character drawn
				i++;
				if (text[i] == 0) {
					break;
				}
				i++;
				if (text[i] == 0) {
					break;
				}
			}
		}
	}
	int chars_drawn = 0;
	int max_width = 0;
	std::string p = text;
	int total_position = 0;
	while (done == false && lines < max_lines) {
		int count = 0;
		int offset = 0;
		std::vector<int> last_offset;
		int max = 0;
		int this_w = 0;
		int chars_drawn_this_time = 0;
		last_offset.push_back(offset);
		Uint32 ch = util::utf8_char_next(p, offset);
		bool set_full = false;
		bool any_spaces = false;
		int skip_next = 0;
		bool skip_check = false;
		while (ch) {
			if (ch == ' ') {
				any_spaces = true;
			}
			if (interpret_colour_codes && ch == '|') {
				skip_next = 2;
			}
			else if (interpret_extra_glyphs && ch == '@') {
				skip_next = 2;
			}
			else if (ch != '^' && ch != '$') {
				if (skip_next > 0) {
					skip_next--;
				}
				else {
					std::map<Uint32, Glyph *>::iterator it = glyphs.find(ch);
					if (it == glyphs.end()) {
						this_w += 5;
					}
					else {
						std::pair<Uint32, Glyph *> pair = *it;
						Glyph *glyph = pair.second;
						this_w += glyph->size.w;
					}
				}
			}
			int line_max_w = (w == INT_MAX) ? w : w+1; // + 1 because the last character will have no advance (see get_text_width)
			if (lines == 0) {
				line_max_w -= first_line_indent;
			}
			if (ch == '^' || ch == '$' || this_w > line_max_w) {
				if (count == 0) {
					if (lines == 0) {
						max = 0;
					}
					else {
						done = true;
					}
				}
				else {
					if (ch == '^') {
						max = count;
						if (set_full) {
							full = true;
						}
					}
					else if (ch == '$') {
						max = count;
					}
					else if (this_w > line_max_w) {
						if (any_spaces && ch != ' ') {
							count--;
							last_offset.pop_back();
							offset = last_offset.back();
						}
						else {
							if (any_spaces == false && ch != ' ' && lines == 0) {
								/*
								lines++;
								curr_y += line_height;
								max = (int)text.length();
								skip_check = true;
								chars_drawn_this_time = max;
								set_full = true;
								*/
								max = 0;
							}
							else {
								max = count;
							}
						}
					}
				}
				break;
			}
			if (ch == ' ') {
				max = count;
			}
			count++;
			last_offset.push_back(offset);
			ch =  util::utf8_char_next(p, offset);
			if (chars_drawn+count < chars_to_draw) {
				chars_drawn_this_time++;
				set_full = true;
			}
			else {
				set_full = false;
			}
		}
		if (skip_check == false && util::utf8_char_offset(p, last_offset.back()) == 0) {
			max = count;
		}
		int old_max = max;
		max = MIN(chars_drawn_this_time, max);
		if (done == false) {
			std::string s = util::utf8_substr(p, 0, max);
			int line_w = get_text_width(strip_codes(s, interpret_colour_codes, true, interpret_extra_glyphs));
			if (line_w > max_width) {
				max_width = line_w;
			}
			if (dry_run == false) {
				float dx = dest_position.x;
				if (lines == 0) {
					dx += first_line_indent;
				}
				colour = draw(colour, s, util::Point<float>(dx, curr_y), interpret_colour_codes, centre);
			}
			total_position += max;
			p = util::utf8_substr(text, total_position);
			Uint32 ch2 = util::utf8_char(p, 0);
			if (ch2 == ' ') {
				total_position++;
				p = util::utf8_substr(text, total_position);
			}
			else if (ch == '^') { // new window
				total_position++;
				done = true;
				// Don't include in printed string
			}
			else if (ch == '$') { // newline
				total_position++;
				p = util::utf8_substr(text, total_position);
			}
			chars_drawn = total_position;
			curr_y += line_height;
			if (max < old_max) {
				done = true;
			}
			else {
				lines++;
				if (lines >= max_lines) {
					full = true;
				}
			}
		}
		if (util::utf8_char(p, 0) == 0) {
			done = true;
			full = true;
		}
		if (chars_drawn >= chars_to_draw) {
			done = true;
		}
	}

	width = max_width;
	num_lines = lines;

	return chars_drawn;
}

void Font::add_extra_glyph(int code, Image *image)
{
	extra_glyphs[code] = image;
}

void Font::remove_extra_glyph(int code)
{
	std::map<int, Image *>::iterator it = extra_glyphs.find(code);
	if (it != extra_glyphs.end()) {
		extra_glyphs.erase(it);
	}
}

void Font::set_extra_glyph_offset(util::Point<float> offset)
{
	extra_glyph_offset = offset;
}

#ifdef _WIN32
void *Font::operator new(size_t i)
{
	return _mm_malloc(i,16);
}

void Font::operator delete(void* p)
{
	_mm_free(p);
}
#endif

} // End namespace gfx

} // End namespace noo
