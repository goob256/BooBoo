#include "shim4/devsettings.h"
#include "shim4/gfx.h"
#include "shim4/json.h"
#include "shim4/shim.h"
#include "shim4/util.h"
#include "shim4/widgets.h"

using namespace noo;

namespace noo {

namespace gui {

DevSettings_GUI::DevSettings_GUI()
{
	Widget *modal_main_widget = new Widget(1.0f, 1.0f);

	list = new DevSettings_List();
	list->set_relative_position(5, 5);
	list->set_parent(modal_main_widget);

	gui = new TGUI(modal_main_widget, shim::screen_size.w, shim::screen_size.h);

	gui->set_focus(list);
}

DevSettings_GUI::~DevSettings_GUI()
{
}

bool DevSettings_GUI::is_editing()
{
	return list->is_editing();
}

void DevSettings_GUI::update()
{
	if (list->is_done()) {
		exit();
	}
}

//--

DevSettings_NumGetter_GUI::DevSettings_NumGetter_GUI(std::string text, std::string initial, bool decimals_allowed, util::Callback callback, void *callback_data) :
	callback(callback),
	callback_data(callback_data),
	_character(0),
	decimals_allowed(decimals_allowed)
{
	Widget *modal_main_widget = new Widget(1.0f, 1.0f);
	modal_main_widget->bg_colour.r = 0;
	modal_main_widget->bg_colour.g = 0;
	modal_main_widget->bg_colour.b = 0;
	modal_main_widget->bg_colour.a = 128;

	Widget *container = new Widget(150, 60);
	container->set_centre_x(true);
	container->set_centre_y(true);
	container->set_parent(modal_main_widget);

	container->bg_colour.r = shim::interface_bg.r;
	container->bg_colour.g = shim::interface_bg.g;
	container->bg_colour.b = shim::interface_bg.b;
	container->bg_colour.a = shim::interface_bg.a;

	header = new DevSettings_Label(text);
	header->set_padding_top(5);
	header->set_centre_x(true);
	header->set_parent(container);

	number = new DevSettings_Label(initial);
	number->set_padding_top(20);
	number->set_centre_x(true);
	number->set_clear_float_x(true);
	number->set_parent(container);

	character = new DevSettings_Label(get_character_text());
	character->set_padding_top(35);
	character->set_centre_x(true);
	character->set_clear_float_x(true);
	character->set_parent(container);

	gui = new TGUI(modal_main_widget, shim::screen_size.w, shim::screen_size.h);
}

DevSettings_NumGetter_GUI::~DevSettings_NumGetter_GUI()
{
}

void DevSettings_NumGetter_GUI::handle_event(TGUI_Event *event)
{
	char chars[12] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '.' };
	int num_chars = decimals_allowed ? 12 : 11;

	if ((event->type == TGUI_KEY_DOWN && event->keyboard.code == TGUIK_RETURN) || (event->type == TGUI_JOY_DOWN && event->joystick.button == TGUI_B_A)) {
		std::string s = number->get_text();
		char s2[2];
		s2[0] = chars[_character];
		s2[1] = 0;
		s += s2;
		number->set_text(s);
		gui->layout();
	}
	else if ((event->type == TGUI_KEY_DOWN && event->keyboard.code == TGUIK_ESCAPE) || (event->type == TGUI_JOY_DOWN && event->joystick.button == TGUI_B_B)) {
		Callback_Data d;
		d.text = number->get_text();
		d.userdata = callback_data;
		callback(&d);
		exit();
	}
	else if ((event->type == TGUI_KEY_DOWN && event->keyboard.code == TGUIK_BACKSPACE) || (event->type == TGUI_JOY_DOWN && event->joystick.button == TGUI_B_X)) {
		std::string s = number->get_text();
		if (s != "") {
			s = s.substr(0, s.length()-1);
			number->set_text(s);
		}
		gui->layout();
	}
	else if (event->type == TGUI_FOCUS && event->focus.type == TGUI_FOCUS_RIGHT) {
		_character++;
		_character %= num_chars;
		character->set_text(get_character_text());
		gui->layout();
	}
	else if (event->type == TGUI_FOCUS && event->focus.type == TGUI_FOCUS_LEFT) {
		_character--;
		if (_character < 0) {
			_character = num_chars-1;
		}
		character->set_text(get_character_text());
		gui->layout();
	}
}

std::string DevSettings_NumGetter_GUI::get_character_text()
{
	int white = 0;
	int highlight = 0;

	int closest = INT_MAX;

	// Find colours closest to white and yellow

	for (int i = 0; i < 256; i++) {
		int diff_r = 255 - shim::palette[i].r;
		int diff_g = 255 - shim::palette[i].g;
		int diff_b = 255 - shim::palette[i].b;
		int max = MAX(diff_r, diff_g);
		max = MAX(max, diff_b);
		if (max < closest) {
			white = i;
			closest = max;
		}
	}

	closest = INT_MAX;

	for (int i = 0; i < 256; i++) {
		int diff_r = 255 - shim::palette[i].r;
		int diff_g = 255 - shim::palette[i].g;
		int diff_b = shim::palette[i].b; // not minus anything, yellow.b = 0
		int max = MAX(diff_r, diff_g);
		max = MAX(max, diff_b);
		if (max < closest) {
			highlight = i;
			closest = max;
		}
	}

	char buf[100];

	sprintf(buf, "|%02x", white);
	std::string white_s = util::uppercase(buf);

	sprintf(buf, "|%02x", highlight);
	std::string highlight_s = util::uppercase(buf);

	std::string letters;
	if (decimals_allowed) {
		letters = "0123456789-.";
	}
	else {
		letters = "0123456789-";
	}

	std::string s = white_s;
	s += letters.substr(0, _character);
	s += highlight_s;
	s += letters.substr(_character, 1);
	s += white_s;
	s += letters.substr(_character+1);

	return s;
}

} // End namespace gui

} // End namespace noo
