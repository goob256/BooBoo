#include "shim4/devsettings.h"
#include "shim4/font.h"
#include "shim4/gfx.h"
#include "shim4/gui.h"
#include "shim4/primitives.h"
#include "shim4/shim.h"
#include "shim4/util.h"
#include "shim4/widgets.h"

using namespace noo;

namespace noo {

namespace gui {

static void joystick_edit_callback(void *data)
{
	auto d = static_cast<DevSettings_NumGetter_GUI::Callback_Data *>(data);
	auto list = static_cast<DevSettings_List *>(d->userdata);
	list->set_value(d->text);
}

Widget::Widget(int w, int h) :
	TGUI_Widget(w, h)
{
	start();
}

Widget::Widget(float percent_w, float percent_h) :
	TGUI_Widget(percent_w, percent_h)
{
	start();
}

Widget::Widget(int w, float percent_h) :
	TGUI_Widget(w, percent_h)
{
	start();
}

Widget::Widget(float percent_w, int h) :
	TGUI_Widget(percent_w, h)
{
	start();
}

Widget::Widget(TGUI_Widget::Fit fit, int other) :
	TGUI_Widget(fit, other)
{
	start();
}

Widget::Widget(TGUI_Widget::Fit fit, float percent_other) :
	TGUI_Widget(fit, percent_other)
{
}

Widget::Widget() :
	TGUI_Widget()
{
	start();
}

Widget::~Widget()
{
}

void Widget::start()
{
	bg_colour = shim::transparent;
}

void Widget::draw()
{
	// This is used to clear the background to a darker colour, so don't do it unless this widget is part of
	// the topmost gui because it could happen twice giving a darker colour

	if (shim::guis.size() > 0) {
		TGUI_Widget *root = shim::guis.back()->gui->get_main_widget();
		TGUI_Widget *w = this;
		TGUI_Widget *parent;
		while ((parent = w->get_parent()) != NULL) {
			w = parent;
		}
		if (root != w) {
			return;
		}
	}

	if (bg_colour.a != 0) {
		glm::mat4 old_mv, old_p;
		gfx::get_matrices(old_mv, old_p);
		gfx::set_default_projection(shim::real_screen_size, shim::screen_offset, shim::scale);
		gfx::update_projection();
		gfx::draw_filled_rectangle(bg_colour, {float(calculated_x), float(calculated_y)}, {float(calculated_w), float(calculated_h)});
		gfx::set_matrices(old_mv, old_p);
		gfx::update_projection();
	}
}

//--

DevSettings_List::DevSettings_List() :
	Widget(0, 0),
	top(0),
	selected(0),
	editing(false),
	done(false)
{
	node = shim::shim_json->get_root();
	set_nr();
	row_h = shim::font->get_height() + 1;
	w = shim::devsettings_max_width;
	h = nr * row_h;
	change_time = SDL_GetTicks();
	val_width = w / 4;
}

DevSettings_List::~DevSettings_List()
{
}

void DevSettings_List::draw()
{
	int total_h = row_h * nr;

	// draw the bg/highlight
	gfx::draw_primitives_start();
	gfx::draw_filled_rectangle(shim::interface_bg, {float(calculated_x), float(calculated_y)}, {float(shim::devsettings_max_width), float(total_h)});
	gfx::draw_filled_rectangle(shim::interface_highlight, util::Point<int>(calculated_x, calculated_y) + util::Point<int>(0, (selected-top)*row_h), {float(shim::devsettings_max_width), float(row_h)});
	gfx::draw_primitives_end();

	// draw the name/values

	for (int i = 0; i < nr; i++) {
		int r = i + top;
		util::JSON::Node *n = node->children[r];
		std::string key = util::JSON::trim_quotes(n->key);
		n->update_value();
		std::string value = util::JSON::trim_quotes(n->value);
		if (r == selected) {
			draw_text_scroll(shim::interface_text, key, util::Point<int>(calculated_x, calculated_y) + util::Point<int>(1, i*row_h), shim::devsettings_max_width-(val_width+8));
			if (editing) {
				draw_edit(tmpval, util::Point<int>(calculated_x, calculated_y) + util::Point<int>(shim::devsettings_max_width-(val_width+2), i*row_h), val_width);
			}
			else {
				draw_text_scroll(shim::interface_text, value, util::Point<int>(calculated_x, calculated_y) + util::Point<int>(shim::devsettings_max_width-(val_width+2), i*row_h), (val_width+2));
			}
		}
		else {
			draw_text_clamped(shim::interface_text, key, util::Point<int>(calculated_x, calculated_y) + util::Point<int>(1, i*row_h), shim::devsettings_max_width-(val_width+8));
			draw_text_clamped(shim::interface_text, value, util::Point<int>(calculated_x, calculated_y) + util::Point<int>(shim::devsettings_max_width-(val_width+2), i*row_h), val_width);
		}
	}
}

void DevSettings_List::handle_event(TGUI_Event *event)
{
	if (event->type == TGUI_FOCUS) {
		switch (event->focus.type) {
			case TGUI_FOCUS_LEFT:
				event->keyboard.code = TGUIK_LEFT;
				break;
			case TGUI_FOCUS_RIGHT:
				event->keyboard.code = TGUIK_RIGHT;
				break;
			case TGUI_FOCUS_UP:
				event->keyboard.code = TGUIK_UP;
				break;
			default:
				event->keyboard.code = TGUIK_DOWN;
				break;
		}
		event->type = TGUI_KEY_DOWN;
		event->keyboard.simulated = true;
	}

	bool use_numgetter = false;

	if (event->type == TGUI_JOY_DOWN) {
		if (event->joystick.button == 0) {
			event->type = TGUI_KEY_DOWN;
			event->keyboard.code = TGUIK_RETURN;
			event->keyboard.simulated = true;
			use_numgetter = true;
		}
		else if (event->joystick.button == 1) {
			event->type = TGUI_KEY_DOWN;
			event->keyboard.code = TGUIK_ESCAPE;
			event->keyboard.simulated = true;
		}
	}

	if (event->type == TGUI_KEY_DOWN) {
		if (editing == false && event->keyboard.code == shim::key_u) {
			if (selected > 0) {
				selected--;
				if (selected < top) {
					top = selected;
				}
				change_time = SDL_GetTicks();
			}
		}
		else if (editing == false && event->keyboard.code == shim::key_d) {
			if (selected < (int)node->children.size()-1) {
				selected++;
				if (selected-top >= nr) {
					top++;
				}
				change_time = SDL_GetTicks();
			}
		}
		else if (event->keyboard.code == TGUIK_RETURN) {
			util::JSON::Node *n = node->children[selected];
			std::string v = n->value;
			if (v == "[array]" || v == "[hash]") {
				node = n;
				set_nr();
				shim::guis.back()->gui->layout();
				top = 0;
				selected = 0;
			}
			else {
				if (n->readonly) {
					gfx::add_notification("Read only...");
				}
				else {
					switch (n->type) {
						case util::JSON::Node::BOOL:
							n->set_bool(!n->as_bool());
							break;
						default:
							if (editing) {
								save_edit();
								editing = false;
							}
							else {
								if (use_numgetter && n->type != util::JSON::Node::STRING) {
									std::string initial;
									bool decimals_allowed = false;
									if (n->type == util::JSON::Node::FLOAT) {
										float f = n->as_float();
										char s[1000];
										snprintf(s, 1000, "%f", f);
										initial = s;
										decimals_allowed = true;
									}
									else if (n->type == util::JSON::Node::BYTE) {
										int b = n->as_byte();
										initial = util::itos(b);
									}
									else {
										int i = n->as_int();
										initial = util::itos(i);
									}
									auto gui = new DevSettings_NumGetter_GUI(n->key, initial, decimals_allowed, joystick_edit_callback, this);
									shim::guis.push_back(gui);
								}
								else if (use_numgetter == false) {
									editing = true;
									if (n->type == util::JSON::Node::STRING) {
										tmpval = util::JSON::trim_quotes(n->value);
									}
									else {
										tmpval = n->value;
									}
									cursor_pos = (int)tmpval.length();
									if (cursor_pos == 0) {
										edit_offset = 0;
									}
									else {
										edit_offset = 0; // in case it doesn't set anything (fits)
										set_edit_offset();
									}
								}
							}
							break;
					}
				}
			}
		}
		else if (event->keyboard.code == TGUIK_ESCAPE) {
			if (editing) {
				editing = false;
			}
			else {
				if (node->parent != NULL) {
					node = node->parent;
					set_nr();
					shim::guis.back()->gui->layout();
					top = 0;
					selected = 0;
				}
				else {
					done = true;
				}
			}
		}
		else if (editing && event->keyboard.code == TGUIK_BACKSPACE && cursor_pos != 0) {
			std::string v;
			v = tmpval.substr(0, cursor_pos-1);
			if (cursor_pos < (int)tmpval.length()) {
				v += tmpval.substr(cursor_pos);
			}
			tmpval = v;
			cursor_pos--;
			if (cursor_pos < edit_offset) {
				edit_offset--;
			}
		}
		else if (editing && event->keyboard.code == TGUIK_DELETE && cursor_pos != (int)tmpval.length() && tmpval.length() != 0) {
			std::string v;
			if (cursor_pos != 0) {
				v = tmpval.substr(0, cursor_pos);
			}
			v += tmpval.substr(cursor_pos+1);
			tmpval = v;
		}
		else if (editing && event->keyboard.code == TGUIK_LEFT && cursor_pos != 0) {
			cursor_pos--;
			if (cursor_pos < edit_offset) {
				edit_offset--;
			}
		}
		else if (editing && event->keyboard.code == TGUIK_RIGHT && cursor_pos != (int)tmpval.length()) {
			cursor_pos++;
			int tmp = edit_offset;
			set_edit_offset();
			if (tmp > edit_offset) {
				edit_offset = tmp;
			}
		}
	}
	else if (editing && event->type == TGUI_TEXT) {
		util::JSON::Node *n = node->children[selected];
		if (n->type == util::JSON::Node::STRING) {
			// FIXME: support quotes
			if (event->text.text[0] != '"') {
				insert_text(event->text.text);
			}
		}
		else {
			if (n->type == util::JSON::Node::FLOAT && tmpval.find('.') == std::string::npos && event->text.text[0] == '.') {
				insert_text(event->text.text);
			}
			else if (isdigit(event->text.text[0])) {
				insert_text(event->text.text);
			}
		}
	}
}

void DevSettings_List::set_nr()
{
	nr = MIN(shim::devsettings_num_rows, (int)node->children.size());
}

void DevSettings_List::draw_text_clamped(SDL_Colour colour, std::string text, util::Point<int> pos, int max_width)
{
	if (text.length() == 0) {
		return;
	}

	if (shim::font->get_text_width(text) <= max_width) {
		shim::font->draw(colour, text, pos);
		return;
	}

	int len = int(text.length())-1;
	const std::string ellipses = "...";

	while (len > 0 && shim::font->get_text_width(text.substr(0, len) + ellipses) > max_width) {
		len--;
	}

	std::string new_text = text.substr(0, len) + ellipses;

	shim::font->draw(colour, new_text, pos);
}

void DevSettings_List::draw_text_scroll(SDL_Colour colour, std::string text, util::Point<int> pos, int max_width)
{
	if (text.length() == 0) {
		return;
	}

	if (shim::font->get_text_width(text) <= max_width) {
		shim::font->draw(colour, text, pos);
		return;
	}

	int end = 0;
	const std::string ellipses = "...";

	while (end < (int)text.length() && shim::font->get_text_width(ellipses + text.substr(end) + ellipses) > max_width) {
		end++;
	}

	const int scroll = 200;
	const int pause = 1000;
	int duration = end * scroll;

	int t = (SDL_GetTicks()-change_time) % (duration + pause*2);

	int o;
	if (t < pause) {
		o = 0;
	}
	else if (t >= pause+duration) {
		o = end;
	}
	else {
		o = (t-pause) / scroll;
	}

	int len;
	
	if (t >= pause+duration) {
		len = int(text.length()) - end;
	}
	else {
		len = 1;
		while (len < (int)text.length()-o && shim::font->get_text_width(ellipses + text.substr(o, len) + ellipses) < max_width) {
			len++;
		}
		len--;
	}

	std::string new_text;

	if (o == 0) {
		pos.x += shim::font->get_text_width(ellipses) + 1;
	}
	else {
		new_text = ellipses;
	}
	new_text += text.substr(o, len);
	if (t < duration+pause) {
		new_text += ellipses;
	}

	shim::font->draw(colour, new_text, pos);
}

void DevSettings_List::save_edit()
{
	util::JSON::Node *n = node->children[selected];
	switch (n->type) {
		case util::JSON::Node::INT:
			n->set_int(atoi(tmpval.c_str()));
			break;
		case util::JSON::Node::FLOAT:
			n->set_float(atof(tmpval.c_str()));
			break;
		case util::JSON::Node::BYTE:
			n->set_byte(atoi(tmpval.c_str()));
			break;
		default: // STRING
			n->set_string(tmpval);
			break;
	}
}

void DevSettings_List::draw_edit(std::string text, util::Point<int> pos, int max_width)
{
	util::Point<int> bb_pos = pos + util::Point<int>(0, 1);
	util::Size<int> bb_sz = util::Size<int>(max_width, row_h-2);
	pos.x += 1;

	gfx::draw_filled_rectangle(shim::interface_edit_bg, bb_pos, bb_sz);

	gfx::set_scissor(bb_pos.x, bb_pos.y, bb_sz.w, bb_sz.h);

	shim::font->draw(shim::interface_edit_fg, text.substr(edit_offset), pos);

	Uint32 t = SDL_GetTicks() % 1000;
	
	if (t < 500) {
		int o = cursor_pos - edit_offset;

		std::string c = text.substr(edit_offset, o);

		int w = shim::font->get_text_width(c);

		float x = pos.x + w + 1;

		gfx::draw_line(shim::interface_bg, {x, bb_pos.y + 1.0f}, {x, bb_pos.y + bb_sz.h - 3.0f});
	}

	gfx::unset_scissor();
}

void DevSettings_List::insert_text(char *text)
{
	std::string v = tmpval.substr(0, cursor_pos);
	v += std::string(text);
	v += tmpval.substr(cursor_pos);
	tmpval = v;
	cursor_pos += strlen(text); // FIXME: utf-8
	int tmp = edit_offset;
	set_edit_offset();
	if (tmp > edit_offset) {
		edit_offset = tmp;
	}
}

void DevSettings_List::set_edit_offset()
{
	if (shim::font->get_text_width(tmpval.substr(edit_offset, cursor_pos-edit_offset)) < val_width) {
		return;
	}
	int o = cursor_pos - 1;
	while (o > 0 && shim::font->get_text_width(tmpval.substr(o, cursor_pos-o)) < val_width-2) {
		o--;
	}
	o++;
	edit_offset = o;
}

bool DevSettings_List::is_editing()
{
	return editing;
}

void DevSettings_List::set_value(std::string text)
{
	util::JSON::Node *n = node->children[selected];
	if (n->type == util::JSON::Node::FLOAT) {
		float v = atof(text.c_str());
		n->set_float(v);
	}
	else if (n->type == util::JSON::Node::BYTE) {
		int v = atoi(text.c_str());
		n->set_byte(v);
	}
	else {
		int v = atoi(text.c_str());
		n->set_int(v);
	}
}

bool DevSettings_List::is_done()
{
	return done;
}

//--

DevSettings_Label::DevSettings_Label(std::string text) :
	Widget(shim::font->get_text_width(text), shim::font->get_height()),
	text(text)
{
}

DevSettings_Label::~DevSettings_Label()
{
}

void DevSettings_Label::draw()
{
	shim::font->draw(shim::white, text, {float(calculated_x), float(calculated_y)});
}

void DevSettings_Label::set_text(std::string text)
{
	this->text = text;
	w = shim::font->get_text_width(text);
}

std::string DevSettings_Label::get_text()
{
	return text;
}

} // End namespace gui

} // End namespace noo
