// leaks memory like a sieve!

#include <vector>
#include <string>
#include <cmath>

#define ALLEGRO_STATICLINK

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>

#define PAD 2

// Closes 'f'
unsigned char *slurp_from_file(ALLEGRO_FILE *f, int *ret_size, bool terminate_with_0, bool use_malloc)
{
	int size = (int)al_fsize(f);
	unsigned char *bytes;
	int extra = terminate_with_0 ? 1 : 0;
	
	if (size < 0) {
		std::vector<char> v;
		int c;
		while ((c = al_fgetc(f)) != EOF) {
			v.push_back(c);
		}
		if (use_malloc) {
			bytes = (unsigned char *)malloc(v.size()+extra);
		}
		else {
			bytes = new unsigned char[v.size()+extra];
		}
		for (unsigned int i = 0; i < v.size(); i++) {
			bytes[i] = v[i];
		}
	}
	else {
		if (use_malloc) {
			bytes = (unsigned char *)malloc(size+extra);
		}
		else {
			bytes = new unsigned char[size+extra];
		}
		al_fread(f, bytes, size);
	}
	al_fclose(f);
	if (extra) {
		bytes[size] = 0;
	}

	if (ret_size)
		*ret_size = size + extra;

	return bytes;
}

std::string utf8_char_to_string(int32_t ch)
{
	unsigned char buf[5];

	int bytes;

	if (ch & 0x001F0000) {
		bytes = 4;
	}
	else if (ch & 0x0000F800) {
		bytes = 3;
	}
	else if (ch & 0x00000780) {
		bytes = 2;
	}
	else {
		bytes = 1;
	}

	if (bytes == 1) {
		buf[0] = (unsigned char)ch;
	}
	else if (bytes == 2) {
		buf[1] = (ch & 0x3F) | 0x80;
		ch >>= 6;
		buf[0] = (ch & 0x1F) | 0xC0;
	}
	else if (bytes == 3) {
		buf[2] = (ch & 0x3F) | 0x80;
		ch >>= 6;
		buf[1] = (ch & 0x3F) | 0x80;
		ch >>= 6;
		buf[0] = (ch & 0xF) | 0xE0;
	}
	else {
		buf[3] = (ch & 0x3F) | 0x80;
		ch >>= 6;
		buf[2] = (ch & 0x3F) | 0x80;
		ch >>= 6;
		buf[1] = (ch & 0x3F) | 0x80;
		ch >>= 6;
		buf[0] = (ch & 0x7) | 0xF0;
	}

	buf[bytes] = 0;

	return std::string((char *)buf);
}

int main(void)
{
	ALLEGRO_FONT *ttf;

	if (!al_init()) {
		printf("Could not init Allegro.\n");
		exit(1);
	}
	
	al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);

	al_init_font_addon();
	al_init_ttf_addon();
	al_init_image_addon();

	ttf = al_load_font("font.ttf", 16, ALLEGRO_TTF_MONOCHROME | ALLEGRO_TTF_NO_KERNING | ALLEGRO_TTF_NO_AUTOHINT);

	ALLEGRO_FILE *f = al_fopen("glyphs.utf8", "rb");
	int sz;
	unsigned char *glyphs = slurp_from_file(f, &sz, true, false);

	ALLEGRO_USTR *ustr = al_ustr_new((const char *)glyphs);

	int num_glyphs = al_ustr_length(ustr);
	int pos = 0;

	int half = sqrtf((float)num_glyphs) + 1;

	ALLEGRO_FILE *info = al_fopen("info.ini", "r");

	int info_sz;
	unsigned char *info_txt = slurp_from_file(info, &info_sz, true, false);
	while (*info_txt != '=' && *info_txt != 0) {
		info_txt++;
	}
	if (*info_txt == 0) {
		exit(1);
	}
	info_txt++;
	int SIZE = 12;//al_get_font_line_height(ttf);//atoi((const char *)info_txt);
	while (*info_txt != '=' && *info_txt != 0) {
		info_txt++;
	}
	int wmul = 1;
	if (*info_txt != 0) {
		info_txt++;
		bool WIDE = atoi((const char *)info_txt);
		wmul = WIDE ? 2 : 1;
	}

	ALLEGRO_BITMAP *bmp = al_create_bitmap((half * (SIZE*wmul + PAD*2)) * 3, half * (SIZE + PAD*2));

	al_set_target_bitmap(bmp);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));

	for (int i = 0; i < num_glyphs; i++) {
		int32_t glyph = al_ustr_get_next(ustr, &pos);
		if (glyph == '\n' || glyph == '\r') {
			break;
		}
		std::string s = utf8_char_to_string(glyph);
		int x = i % half;
		int y = i / half;
		int bbx, bby, bbw, bbh;
		al_get_text_dimensions(ttf, s.c_str(), &bbx, &bby, &bbw, &bbh);
		int xo = -bbx;
		al_draw_text(
			ttf,
			al_map_rgb(255, 255, 255),
			((x * (SIZE*wmul + PAD*2)) * 3 + 2 + xo),
			y * (SIZE + PAD*2) + 3, // extra +2 for first row
			0,
			s.c_str()
		);
		int w = (SIZE*wmul + PAD*2);
		// drop shadow
		al_draw_text(
			ttf,
			al_map_rgb(255, 255, 255),
			((x * (SIZE*wmul + PAD*2)) * 3 + 2 + w + 1 + xo),
			y * (SIZE + PAD*2) + 3, // extra +2 for first row
			0,
			s.c_str()
		);
		al_draw_text(
			ttf,
			al_map_rgb(255, 255, 255),
			((x * (SIZE*wmul + PAD*2)) * 3 + 2 + w + 1 + xo),
			y * (SIZE + PAD*2) + 3 + 1, // extra +2 for first row
			0,
			s.c_str()
		);
		al_draw_text(
			ttf,
			al_map_rgb(255, 255, 255),
			((x * (SIZE*wmul + PAD*2)) * 3 + 2 + w + xo),
			y * (SIZE + PAD*2) + 3 + 1, // extra +2 for first row
			0,
			s.c_str()
		);
		// full shadow
		al_draw_text(
			ttf,
			al_map_rgb(255, 255, 255),
			((x * (SIZE*wmul + PAD*2)) * 3 + 2 + w*2 + 1 + xo),
			y * (SIZE + PAD*2) + 3, // extra +2 for first row
			0,
			s.c_str()
		);
		al_draw_text(
			ttf,
			al_map_rgb(255, 255, 255),
			((x * (SIZE*wmul + PAD*2)) * 3 + 2 + w*2 - 1 + xo),
			y * (SIZE + PAD*2) + 3, // extra +2 for first row
			0,
			s.c_str()
		);
		al_draw_text(
			ttf,
			al_map_rgb(255, 255, 255),
			((x * (SIZE*wmul + PAD*2)) * 3 + 2 + w*2 + xo),
			y * (SIZE + PAD*2) + 3 + 1, // extra +2 for first row
			0,
			s.c_str()
		);
		al_draw_text(
			ttf,
			al_map_rgb(255, 255, 255),
			((x * (SIZE*wmul + PAD*2)) * 3 + 2 + w*2 + xo),
			y * (SIZE + PAD*2) + 3 - 1, // extra +2 for first row
			0,
			s.c_str()
		);
		al_draw_text(
			ttf,
			al_map_rgb(255, 255, 255),
			((x * (SIZE*wmul + PAD*2)) * 3 + 2 + w*2 + 1 + xo),
			y * (SIZE + PAD*2) + 3 + 1, // extra +2 for first row
			0,
			s.c_str()
		);
		al_draw_text(
			ttf,
			al_map_rgb(255, 255, 255),
			((x * (SIZE*wmul + PAD*2)) * 3 + 2 + w*2 - 1 + xo),
			y * (SIZE + PAD*2) + 3 - 1, // extra +2 for first row
			0,
			s.c_str()
		);
		al_draw_text(
			ttf,
			al_map_rgb(255, 255, 255),
			((x * (SIZE*wmul + PAD*2)) * 3 + 2 + w*2 + 1 + xo),
			y * (SIZE + PAD*2) + 3 - 1, // extra +2 for first row
			0,
			s.c_str()
		);
		al_draw_text(
			ttf,
			al_map_rgb(255, 255, 255),
			((x * (SIZE*wmul + PAD*2)) * 3 + 2 + w*2 - 1 + xo),
			y * (SIZE + PAD*2) + 3 + 1, // extra +2 for first row
			0,
			s.c_str()
		);
	}

	al_save_bitmap("font.png", bmp);
}
