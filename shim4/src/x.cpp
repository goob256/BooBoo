#include "shim4/error.h"
#include "shim4/tokenizer.h"
#include "shim4/util.h"
#include "shim4/x.h"

#include "shim4/internal/gfx.h"

using namespace noo;

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

struct Button {
	std::string text;
	int x;
	int y;
	int w;
	int h;
};

namespace noo {

namespace gui {

int x_popup(std::string caption, std::string text, Popup_Type type)
{
	if (gfx::internal::gfx_context.inited == false) {
		util::infomsg("%s: %s\n", caption.c_str(), text.c_str());
		return -1;
	}

	std::vector<std::string> lines;
	util::Tokenizer t(text, '^');
	std::string tok;
	while ((tok = t.next()) != "") {
		util::trim(tok);
		if (tok != "") {
			lines.push_back(tok);
		}
	}

	if (lines.size() > 5) { // max 5 lines
		int remove = lines.size() - 5;
		for (int i = 0; i < remove; i++) {
			lines.erase(lines.begin()+lines.size()-1);
		}
	}

	int selected = 0;

	std::vector<Button> buttons;

	if (type == OK) {
		Button b;
		b.text = "OK";
		buttons.push_back(b);
	}
	else if (type == YESNO) {
		Button b;
		b.text = "Yes";
		buttons.push_back(b);
		b.text = "No";
		buttons.push_back(b);
	}
	else {
		return -1;
	}

	Display *d = XOpenDisplay(0);
	Window w;
	GC gc;
	XEvent e;
	int s;
	s = shim::adapter;

	long event_mask = ExposureMask | KeyPressMask | ButtonPressMask;

	int font_size = 16;

	char font_name[200];

	sprintf(font_name, "-*-clean-*-*-*-*-%d-*-*-*-*-*-*-*", font_size);

	XFontStruct *font = XLoadQueryFont(d, font_name);

	if (font == 0) {
		sprintf(font_name, "-bitstream-charter-medium-r-*-*-%d-*-*-*-*-*-*-*", font_size);
		font = XLoadQueryFont(d, font_name);
	}

	if (font == 0) {
		util::errormsg("%s\n", text.c_str());
		return -1;
	}

	#define FONT_SIZE (font->max_bounds.ascent+font->max_bounds.descent)
	#define CENTRE(y) (y+(font->max_bounds.ascent-font->max_bounds.descent)/2)
	
	int max_width = 0;
	for (size_t i = 0; i < lines.size(); i++) {
		max_width = MAX(max_width, XTextWidth(font, lines[i].c_str(), lines[i].length()));
	}

	int button_w = 50;
	int button_h = 25;
	int width = 100 + max_width;
	int height = 150 + ((FONT_SIZE+2) * lines.size()) + button_h;
	int x = 10;
	int y = 10;

	SDL_DisplayMode mode;
	if (SDL_GetCurrentDisplayMode(0, &mode) == 0) {
		x = (mode.w - width) / 2;
		y = (mode.h - height) / 2 * 0.75f;
	}

	w = XCreateSimpleWindow(d, RootWindow(d, s), x, y, width, height, 1, BlackPixel(d, s), WhitePixel(d, s));
	gc = XCreateGC(d, w, 0, 0);	
	XStoreName(d, w, caption.c_str());
	XSelectInput(d, w, event_mask);

	Atom WM_DELETE_WINDOW = XInternAtom(d, "WM_DELETE_WINDOW", False); 
	XSetWMProtocols(d, w, &WM_DELETE_WINDOW, 1);

	XSetTransientForHint(d, w, gfx::internal::gfx_context.x_window);

	Atom atom = XInternAtom(d, "_NET_WM_STATE", False);
	Atom modal = XInternAtom(d, "_NET_WM_STATE_MODAL", False);
	XChangeProperty(d, w, atom, XA_ATOM, 32, PropModeReplace, (unsigned char *)&modal, 1);
	
	XMapWindow(d, w);

	int spacing = 10;
	int button_x = width / 2 - (buttons.size() * button_w + (buttons.size()-1) * spacing) / 2;
	int button_y = height - 50 - button_h;
	int curr_x = button_x;
	for (int i = 0; i < (int)buttons.size(); i++) {
		buttons[i].x = curr_x;
		buttons[i].y = button_y;
		buttons[i].w = button_w;
		buttons[i].h = button_h;
		curr_x += button_w + spacing;
	}

	XSetFont(d, gc, font->fid);

	while (1) {
		if (XCheckMaskEvent(d, event_mask, &e) == False) {
			SDL_Delay(1); // this is so we don't burn CPU cycles
			SDL_PumpEvents();
			SDL_FlushEvents(0, 0xffffffff);
			continue;
		}

		if (e.type == KeyPress) {
			if (e.xkey.keycode == 113 && selected > 0) {
				selected--;
				XEvent ev;
				ev.type = Expose;
				XSendEvent(d, w, false, ExposureMask, &ev);
			}
			else if (e.xkey.keycode == 114 && selected < (int)buttons.size()-1) {
				selected++;
				XEvent ev;
				ev.type = Expose;
				XSendEvent(d, w, false, ExposureMask, &ev);
			}
			else if (e.xkey.keycode == 36) {
				break;
			}
			else if (e.xkey.keycode == 9) {
				if (type == 1) {
					selected = 1;
					break;
				}
			}
		}
		else if (e.type == ButtonPress) {
			int ex = e.xbutton.x;
			int ey = e.xbutton.y;
			bool found = false;
			for (int i = 0; i < (int)buttons.size(); i++) {
				int bx = buttons[i].x;
				int by = buttons[i].y;
				int bw = buttons[i].w;
				int bh = buttons[i].h;
				if (ex >= bx && ex < bx+bw && ey >= by && ey < by+bh) {
					found = true;
					selected = i;
					break;
				}
			}
			if (found) {
				break;
			}
		}
		else if (e.type == Expose) {
			XSetForeground(d, gc, WhitePixel(d, s));
			XFillRectangle(d, w, gc, 0, 0, width, height);
			XSetForeground(d, gc, BlackPixel(d, s));
			for (size_t i = 0; i < lines.size(); i++) {
				std::string t = lines[i];
				int tx = width / 2 - XTextWidth(font, t.c_str(), t.length()) / 2;
				int ty = button_y - 50 - ((FONT_SIZE+2) * lines.size()) + FONT_SIZE + i * (FONT_SIZE+2);
				XDrawString(d, w, gc, tx, ty, t.c_str(), t.length());
			}
			for (int i = 0; i < (int)buttons.size(); i++) {
				unsigned long button_colour, text_colour;
				if (i == selected) {
					button_colour = BlackPixel(d, s);
					text_colour = WhitePixel(d, s);
				}
				else {
					button_colour = WhitePixel(d, s);
					text_colour = BlackPixel(d, s);
				}
				XSetForeground(d, gc, button_colour);
				XFillRectangle(d, w, gc, buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h);
				XSetForeground(d, gc, BlackPixel(d, s));
				XDrawRectangle(d, w, gc, buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h);
				XSetForeground(d, gc, text_colour);
				int tx = buttons[i].x + buttons[i].w/2 - XTextWidth(font, buttons[i].text.c_str(), buttons[i].text.length()) / 2;
				XDrawString(d, w, gc, tx, buttons[i].y + CENTRE(buttons[i].h/2), buttons[i].text.c_str(), buttons[i].text.length());
			}
			XFlush(d);
		}
	}

	XFreeFont(d, font);

	XFreeGC(d, gc);

	XDestroyWindow(d, w);

	XCloseDisplay(d);

	SDL_Delay(100); // this is to give X time to hand all the events to SDL
	SDL_PumpEvents();
	SDL_FlushEvents(0, 0xffffffff);

	return selected;
}

} // End namespace gui

} // End namespace noo
