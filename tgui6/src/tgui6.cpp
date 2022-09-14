#include <cmath>
#include <climits>

#include <algorithm>

#include "tgui6/tgui6.h"

TGUI_Event::TGUI_Event()
{
	type = TGUI_UNKNOWN;

	keyboard.code = 0;
	keyboard.is_repeat = 0;

	mouse.button = 0;
	mouse.x = 0;
	mouse.y = 0;
	mouse.normalised = 0;

	joystick.id = 0;
	joystick.button = 0;
	joystick.axis = 0;
	joystick.value = 0;
	joystick.is_repeat = 0;

	focus.type = TGUI_FOCUS_UNKNOWN;
}

TGUI_Event::TGUI_Event(TGUI_Event const &x)
{
	type = x.type;

	keyboard.code = x.keyboard.code;
	keyboard.is_repeat = x.keyboard.is_repeat;
	keyboard.simulated = x.keyboard.simulated;

	mouse.button = x.mouse.button;
	mouse.x = x.mouse.x;
	mouse.y = x.mouse.y;
	mouse.normalised = x.mouse.normalised;
	mouse.is_touch = x.mouse.is_touch;
	mouse.finger = x.mouse.finger;
	mouse.is_repeat = x.mouse.is_repeat;

	joystick.id = x.joystick.id;
	joystick.button = x.joystick.button;
	joystick.axis = x.joystick.axis;
	joystick.value = x.joystick.value;
	joystick.is_repeat = x.joystick.is_repeat;

	for (int i = 0; i < 32; i++) {
		text.text[i] = x.text.text[i];
	}

	focus.type = x.focus.type;
}

TGUI_Event &TGUI_Event::operator=(const TGUI_Event &x)
{
	if (this == &x)
		return *this;
 
	type = x.type;

	keyboard.code = x.keyboard.code;
	keyboard.is_repeat = x.keyboard.is_repeat;
	keyboard.simulated = x.keyboard.simulated;

	mouse.button = x.mouse.button;
	mouse.x = x.mouse.x;
	mouse.y = x.mouse.y;
	mouse.normalised = x.mouse.normalised;
	mouse.is_touch = x.mouse.is_touch;
	mouse.finger = x.mouse.finger;
	mouse.is_repeat = x.mouse.is_repeat;

	joystick.id = x.joystick.id;
	joystick.button = x.joystick.button;
	joystick.axis = x.joystick.axis;
	joystick.value = x.joystick.value;
	joystick.is_repeat = x.joystick.is_repeat;

	for (int i = 0; i < 32; i++) {
		text.text[i] = x.text.text[i];
	}

	focus.type = x.focus.type;

	return *this;
}
/*
TGUI_Event const &TGUI_Event::operator=(class TGUI_Event const &x)
{
	type = x.type;

	keyboard.code = x.keyboard.code;
	keyboard.is_repeat = x.keyboard.is_repeat;
	keyboard.simulated = x.keyboard.simulated;

	mouse.button = x.mouse.button;
	mouse.x = x.mouse.x;
	mouse.y = x.mouse.y;
	mouse.normalised = x.mouse.normalised;
	mouse.is_touch = x.mouse.is_touch;
	mouse.finger = x.mouse.finger;
	mouse.is_repeat = x.mouse.is_repeat;

	joystick.id = x.joystick.id;
	joystick.button = x.joystick.button;
	joystick.axis = x.joystick.axis;
	joystick.value = x.joystick.value;
	joystick.is_repeat = x.joystick.is_repeat;

	for (int i = 0; i < 32; i++) {
		text.text[i] = x.text.text[i];
	}

	focus.type = x.focus_type;
}
*/

TGUI_Event::~TGUI_Event()
{
}

int TGUI::focus_sloppiness = 2;

void TGUI::set_focus_sloppiness(int sloppiness)
{
	focus_sloppiness = sloppiness;
}

TGUI::TGUI(TGUI_Widget *main_widget, int w, int h) :
	main_widget(main_widget),
	focus(0),
	w(w),
	h(h),
	offset_x(0),
	offset_y(0)
{
	layout();
	focus_something();
}

TGUI::~TGUI()
{
	if (main_widget) {
		destroy(main_widget);
	}
}

void TGUI::destroy(TGUI_Widget *widget)
{
	assert(widget);

	for (size_t i = 0; i < widget->children.size(); i++) {
		destroy(widget->children[i]);
	}

	delete widget;
}

void TGUI::layout()
{
	if (main_widget) {
		reset_size(main_widget);
		// Multiple passes are needed to get FIT_X/FIT_Y widgets right
		while (true) {
			set_sizes(main_widget);
			int passes_needed = set_positions(main_widget, offset_x+main_widget->get_padding_left(), offset_y+main_widget->get_padding_top());
			if (passes_needed == 0) {
				break;
			}
		}
	}
}

void TGUI::resize(int w, int h)
{
	this->w = w;
	this->h = h;
	layout();
}

void TGUI::draw()
{
	if (main_widget) {
		draw(main_widget);
	}
}

void TGUI::handle_event(TGUI_Event *event)
{
	if (main_widget) {
		if (event->type == TGUI_MOUSE_DOWN) {
			TGUI_Widget *widget = get_event_owner(event);
			if (widget && widget->accepts_focus) {
				focus = widget;
			}
		}

		handle_event(event, main_widget);

		// Automatically focus a widget to the left, right, top or below the current one
		if (event->type == TGUI_FOCUS) {
			if (event->focus.type == TGUI_FOCUS_LEFT && focus && focus->left_widget) {
				focus = focus->left_widget;
			}
			else if (event->focus.type == TGUI_FOCUS_RIGHT && focus && focus->right_widget) {
				focus = focus->right_widget;
			}
			else if (event->focus.type == TGUI_FOCUS_UP && focus && focus->up_widget) {
				focus = focus->up_widget;
			}
			else if (event->focus.type == TGUI_FOCUS_DOWN && focus && focus->down_widget) {
				focus = focus->down_widget;
			}
			else {
				TGUI_Widget *start = focus == 0 ? main_widget : focus;
				TGUI_Widget *best = start;
				int best_score = INT_MAX;
				int best_grade = 2;
				int x, y;
				if (event->focus.type == TGUI_FOCUS_LEFT) {
					x = -1;
					y = 0;
				}
				else if (event->focus.type == TGUI_FOCUS_RIGHT) {
					x = 1;
					y = 0;
				}
				else if (event->focus.type == TGUI_FOCUS_UP) {
					x = 0;
					y = -1;
				}
				else {
					x = 0;
					y = 1;
				}
				find_focus(start, best, main_widget, x, y, best_score, best_grade);
				if (focus_sloppiness > 0) {
					if (best_grade == 2) {
						int new_best_score = INT_MAX;
						TGUI_Widget *new_best = start;
						if (x < 0) {
							x = 0;
							y = -1;
						}
						else if (x > 0) {
							x = 0;
							y = 1;
						}
						else if (y < 0) {
							x = 1;
							y = 0;
						}
						else {
							x = -1;
							y = 0;
						}
						find_focus(start, new_best, main_widget, x, y, new_best_score, best_grade);
						if (best_grade == 0) {
							best = new_best;
						}
					}
				}
				if (best_grade <= focus_sloppiness) {
					focus = best;
				}
			}
		}
	}
}

void TGUI::set_focus(TGUI_Widget *widget)
{
	focus = widget;
}

void TGUI::focus_something()
{
	if (main_widget) {
		focus_something(main_widget);
	}
}

void TGUI::set_offset(int offset_x, int offset_y)
{
	this->offset_x = offset_x;
	this->offset_y = offset_y;
	layout();
}

TGUI_Widget *TGUI::get_main_widget()
{
	return main_widget;
}

TGUI_Widget *TGUI::get_focus()
{
	return focus;
}

TGUI_Widget *TGUI::get_event_owner(TGUI_Event *event)
{
	if (main_widget) {
		TGUI_Widget *widget = get_event_owner(event, main_widget);
		return widget;
	}
	else {
		return 0;
	}
}

int TGUI::get_width()
{
	return w;
}

int TGUI::get_height()
{
	return h;
}

bool TGUI_Widget::get_float_left()
{
	return float_left;
}

bool TGUI_Widget::get_float_right()
{
	return float_right;
}

bool TGUI_Widget::get_float_bottom()
{
	return float_bottom;
}

bool TGUI_Widget::get_centre_x()
{
	return centre_x;
}

bool TGUI_Widget::get_centre_y()
{
	return centre_y;
}

bool TGUI_Widget::get_clear_float_x()
{
	return clear_float_x;
}

bool TGUI_Widget::get_clear_float_y()
{
	return clear_float_y;
}

bool TGUI_Widget::get_break_line()
{
	return break_line;
}

bool TGUI_Widget::get_accepts_focus()
{
	return accepts_focus;
}

void TGUI::reset_size(TGUI_Widget *widget)
{
	widget->calculated_w = -1;
	widget->calculated_h = -1;
	widget->fitted_x = false; // set_positions uses these
	widget->fitted_y = false;
	for (size_t i = 0; i < widget->children.size(); i++) {
		reset_size(widget->children[i]);
	}
}

void TGUI::set_sizes(TGUI_Widget *widget)
{
	widget->gui = this;
	int width, height, pad_l, pad_r, pad_t, pad_b;
	tgui_get_size(widget->parent, widget, &width, &height, &pad_l, &pad_r, &pad_t, &pad_b);
	widget->calculated_w = width;
	widget->calculated_h = height;
	widget->calculated_padding_left = pad_l;
	widget->calculated_padding_right = pad_r;
	widget->calculated_padding_top = pad_t;
	widget->calculated_padding_bottom = pad_b;
	for (size_t i = 0; i < widget->children.size(); i++) {
		set_sizes(widget->children[i]);
	}
}

int TGUI::set_positions(TGUI_Widget *widget, int x, int y)
{
	int ret = 0;

	x += widget->offset_x;
	y += widget->offset_y;

	widget->calculated_x = x;
	widget->calculated_y = y;

	int parent_width;
	int parent_height;
	if (widget->fit_x && !widget->fitted_x) {
		parent_width = INT_MAX; // gets re-set below after calculation
	}
	else {
		parent_width = widget->calculated_w + widget->calculated_padding_left + widget->calculated_padding_right;
	}
	if (widget->fit_y && !widget->fitted_y) {
		parent_height = INT_MAX; // gets re-set below after calculation
	}
	else {
		parent_height = widget->calculated_h + widget->calculated_padding_top + widget->calculated_padding_bottom;
	}
	(void)parent_height; // not used at this time

	int max_h = 0;
	int dx = 0;
	int dy = 0;
	int max_x = 0;

	for (size_t i = 0; i < widget->children.size(); i++) {
		TGUI_Widget *d = widget->children[i];

		int width, height;
		int pos_x, pos_y;

		if (d->use_relative_position) {
			width = 0;
			height = 0;
			pos_x = d->relative_x;
			pos_y = d->relative_y;
		}
		else {
			width = d->calculated_w + d->calculated_padding_left + d->calculated_padding_right;
			height = d->calculated_h + d->calculated_padding_top + d->calculated_padding_bottom;

			if (dx + width > parent_width || d->break_line) {
				dx = 0;
				dy += max_h;
				max_h = 0;
			}

			if (dx + width > max_x) {
				max_x = dx + width;
			}

			pos_x = d->get_right_pos();
			pos_y = d->get_bottom_pos();

			if (d->float_left == false && d->float_right == false) {
				pos_x += dx;
			}
			if (d->float_bottom == false) {
				pos_y += dy;
			}
		}

		pos_x += d->get_padding_left();
		pos_y += d->get_padding_top();

		int n = set_positions(d, pos_x+x, pos_y+y);
		ret = n > ret ? n : ret;

		if (d->float_left == false && d->float_right == false && d->centre_x == false && d->use_relative_position == false) {
			dx += width;
		}

		if (d->float_bottom == false && d->centre_y == false && d->use_relative_position == false) {
			max_h = height > max_h ? height : max_h;
		}
	}

	bool is_fit = false;

	if (widget->fit_x && !widget->fitted_x) {
		if (ret == 0) {
			widget->calculated_w = max_x;
			widget->fitted_x = true;
		}
		is_fit = true;
	}
	if (widget->fit_y && !widget->fitted_y) {
		if (ret == 0) {
			widget->calculated_h = dy + max_h;
			widget->fitted_y = true;
		}
		is_fit = true;
	}

	if (is_fit) {
		ret++;
	}

	widget->resize();

	return ret;
}

void TGUI::draw(TGUI_Widget *widget)
{
	widget->draw();
	for (size_t i = 0; i < widget->children.size(); i++) {
		draw(widget->children[i]);
	}
	widget->end_draw();
}

TGUI_Widget *TGUI::get_event_owner(TGUI_Event *event, TGUI_Widget *widget)
{
	for (int i = (int)widget->children.size()-1; i >= 0; i--) { // handle in reverse
		TGUI_Widget *d = get_event_owner(event, widget->children[i]);
		if (d != 0) {
			return d;
		}
	}

	if (event->type == TGUI_MOUSE_DOWN || event->type == TGUI_MOUSE_UP || event->type == TGUI_MOUSE_AXIS) {
		int x, y;

		if (widget->use_relative_position) {
			x = widget->relative_x;
			y = widget->relative_y;
		}
		else {
			x = widget->calculated_x;
			y = widget->calculated_y;
		}

		if (event->mouse.x >= x && event->mouse.x < x+widget->calculated_w && event->mouse.y >= y && event->mouse.y < y+widget->calculated_h) {
			return widget;
		}
	}
	else if (event->type != TGUI_UNKNOWN && widget == focus) {
		return widget;
	}

	return 0;
}

void TGUI::handle_event(TGUI_Event *event, TGUI_Widget *widget)
{
	widget->handle_event(event);

	for (size_t i = 0; i < widget->children.size(); i++) {
		handle_event(event, widget->children[i]);
	}
}

bool TGUI::focus_something(TGUI_Widget *widget)
{
	if (widget->accepts_focus) {
		focus = widget;
		return true;
	}

	for (size_t i = 0; i < widget->children.size(); i++) {
		if (focus_something(widget->children[i])) {
			return true;
		}
	}

	return false;
}

// Returns positive for aligned match, negative for unaligned match
void TGUI::focus_distance(TGUI_Widget *start, TGUI_Widget *widget, int dir_x, int dir_y, int &score, int &grade)
{
	int cx = start->calculated_x + start->calculated_w / 2;
	int cy = start->calculated_y + start->calculated_h / 2;

	int box_x1 = cx;
	int box_x2 = cx + dir_x * 1000000;
	int box_y1 = cy;
	int box_y2 = cy + dir_y * 1000000;

	if (box_x1 == box_x2) {
		box_x1 = start->calculated_x;
		box_x2 = box_x1 + start->calculated_w;
	}
	else {
		box_y1 = start->calculated_y;
		box_y2 = box_y1 + start->calculated_h;
	}

	if (box_x2 < box_x1) {
		int tmp = box_x1;
		box_x1 = box_x2;
		box_x2 = tmp;
	}

	if (box_y2 < box_y1) {
		int tmp = box_y1;
		box_y1 = box_y2;
		box_y2 = tmp;
	}

	int widget_x1 = widget->calculated_x;
	int widget_x2 = widget_x1 + widget->calculated_w;
	int widget_y1 = widget->calculated_y;
	int widget_y2 = widget_y1 + widget->calculated_h;

	int widget_cx = (widget_x1 + widget_x2) / 2;
	int widget_cy = (widget_y1 + widget_y2) / 2;

	int dx = widget_cx - cx;
	int dy = widget_cy - cy;
	int dist = int(sqrtf(float(dx*dx + dy*dy)));

	if (!((widget_x1 >= box_x2) || (widget_x2 <= box_x1) || (widget_y1 >= box_y2) || (widget_y2 <= box_y1))) {
		grade = 0;
	}
	else if (((dir_x < 0) && (widget_cx < cx)) || ((dir_x > 0) && (widget_cx > cx)) || ((dir_y < 0) && (widget_cy < cy)) || ((dir_y > 0) && (widget_cy > cy))) {
		grade = 1;
	}
	else {
		grade = 2;
	}

	score = dist;
}

void TGUI::find_focus(TGUI_Widget *start, TGUI_Widget *&current_best, TGUI_Widget *widget, int dir_x, int dir_y, int &best_score, int &best_grade)
{
	if (widget->accepts_focus && widget != start) {
		int score, grade;
		focus_distance(start, widget, dir_x, dir_y, score, grade);
		if (grade < best_grade) {
			best_score = score;
			best_grade = grade;
			current_best = widget;
		}
		else if (grade == best_grade && score < best_score) {
			best_score = score;
			best_grade = grade;
			current_best = widget;
		}
	}

	for (size_t i = 0; i < widget->children.size(); i++) {
		find_focus(start, current_best, widget->children[i], dir_x, dir_y, best_score, best_grade);
	}
}

TGUI_Widget::TGUI_Widget(int w, int h) :
	fit_x(false),
	fit_y(false),
	percent_x(false),
	percent_y(false),
	w(w),
	h(h)
{
	init();
}

TGUI_Widget::TGUI_Widget(float percent_w, float percent_h) :
	fit_x(false),
	fit_y(false),
	percent_x(true),
	percent_y(true),
	percent_w(percent_w),
	percent_h(percent_h)
{
	init();
}

TGUI_Widget::TGUI_Widget(int w, float percent_h) :
	fit_x(false),
	fit_y(false),
	percent_x(false),
	percent_y(true),
	percent_h(percent_h),
	w(w)
{
	init();
}

TGUI_Widget::TGUI_Widget(float percent_w, int h) :
	fit_x(false),
	fit_y(false),
	percent_x(true),
	percent_y(false),
	percent_w(percent_w),
	h(h)
{
	init();
}

TGUI_Widget::TGUI_Widget(Fit fit, int other)
{
	if (fit == FIT_X) {
		fit_x = true;
		fit_y = false;
		w = 0;
		h = other;
	}
	else {
		fit_x = false;
		fit_y = true;
		w = other;
		h = 0;
	}

	percent_x = false;
	percent_y = false;

	init();
}

TGUI_Widget::TGUI_Widget(Fit fit, float percent_other)
{
	if (fit == FIT_X) {
		fit_x = true;
		fit_y = false;
		percent_x = false;
		percent_y = true;
		percent_h = percent_other;
	}
	else {
		fit_x = false;
		fit_y = true;
		percent_x = true;
		percent_y = false;
		percent_w = percent_other;
	}

	w = 0;
	h = 0;

	init();
}

TGUI_Widget::TGUI_Widget()
{
	fit_x = true;
	fit_y = true;
	percent_x = false;
	percent_y = false;

	w = 0;
	h = 0;

	init();
}

TGUI_Widget::~TGUI_Widget()
{
	// can't set_parent here, don't try it (see destroy above, iterators will become invalidated)
}

void TGUI_Widget::draw()
{
}

void TGUI_Widget::end_draw()
{
}

void TGUI_Widget::handle_event(TGUI_Event *event)
{
}

void TGUI_Widget::resize()
{
}

void TGUI_Widget::set_parent(TGUI_Widget *widget)
{
	if (parent) {
		std::vector<TGUI_Widget *>::iterator it;
		if ((it = std::find(parent->children.begin(), parent->children.end(), this)) != parent->children.end()) {
			parent->children.erase(it);
		}
	}

	parent = widget;

	if (parent) {
		parent->children.push_back(this);
	}
}

void TGUI_Widget::set_width(int width)
{
	w = width;
	percent_x = false;
}

void TGUI_Widget::set_width(float percent_width)
{
	percent_w = percent_width;
	percent_x = true;
}

void TGUI_Widget::set_height(int height)
{
	h = height;
	percent_y = false;
}

void TGUI_Widget::set_height(float percent_height)
{
	percent_h = percent_height;
	percent_y = true;
}

void TGUI_Widget::set_padding_left(int padding)
{
	padding_left = padding;
	use_percent_padding_left = false;
}

void TGUI_Widget::set_padding_left(float percent_padding)
{
	percent_padding_left = percent_padding;
	use_percent_padding_left = true;
}

void TGUI_Widget::set_padding_right(int padding)
{
	padding_right = padding;
	use_percent_padding_right = false;
}

void TGUI_Widget::set_padding_right(float percent_padding)
{
	percent_padding_right = percent_padding;
	use_percent_padding_right = true;
}

void TGUI_Widget::set_padding_top(int padding)
{
	padding_top = padding;
	use_percent_padding_top = false;
}

void TGUI_Widget::set_padding_top(float percent_padding)
{
	percent_padding_top = percent_padding;
	use_percent_padding_top = true;
}

void TGUI_Widget::set_padding_bottom(int padding)
{
	padding_bottom = padding;
	use_percent_padding_bottom = false;
}

void TGUI_Widget::set_padding_bottom(float percent_padding)
{
	percent_padding_bottom = percent_padding;
	use_percent_padding_bottom = true;
}

void TGUI_Widget::set_padding(int padding)
{
	padding_left = padding_right = padding_top = padding_bottom = padding;
	use_percent_padding_left = use_percent_padding_right = use_percent_padding_top = use_percent_padding_bottom = false;
}

void TGUI_Widget::set_padding(float percent_padding)
{
	percent_padding_left = percent_padding_right = percent_padding_top = percent_padding_bottom = percent_padding;
	use_percent_padding_left = use_percent_padding_right = use_percent_padding_top = use_percent_padding_bottom = true;
}

void TGUI_Widget::set_relative_position(int relative_x, int relative_y)
{
	use_relative_position = true;
	this->relative_x = relative_x;
	this->relative_y = relative_y;
}

void TGUI_Widget::set_offset_x(int offset_x)
{
	this->offset_x = offset_x;
}

void TGUI_Widget::set_offset_y(int offset_y)
{
	this->offset_y = offset_y;
}

void TGUI_Widget::set_left_widget(TGUI_Widget *widget)
{
	left_widget = widget;
}

void TGUI_Widget::set_right_widget(TGUI_Widget *widget)
{
	right_widget = widget;
}

void TGUI_Widget::set_up_widget(TGUI_Widget *widget)
{
	up_widget = widget;
}

void TGUI_Widget::set_down_widget(TGUI_Widget *widget)
{
	down_widget = widget;
}

void TGUI_Widget::set_float_left(bool float_left)
{
	this->float_left = float_left;
}

void TGUI_Widget::set_float_right(bool float_right)
{
	this->float_right = float_right;
}

void TGUI_Widget::set_float_bottom(bool float_bottom)
{
	this->float_bottom = float_bottom;
}

void TGUI_Widget::set_centre_x(bool centre_x)
{
	this->centre_x = centre_x;
}

void TGUI_Widget::set_centre_y(bool centre_y)
{
	this->centre_y = centre_y;
}

void TGUI_Widget::set_clear_float_x(bool clear_float_x)
{
	this->clear_float_x = clear_float_x;
}

void TGUI_Widget::set_clear_float_y(bool clear_float_y)
{
	this->clear_float_y = clear_float_y;
}

void TGUI_Widget::set_break_line(bool break_line)
{
	this->break_line = break_line;
}

void TGUI_Widget::set_accepts_focus(bool accepts_focus)
{
	this->accepts_focus = accepts_focus;
	if (accepts_focus == false) {
		if (gui->get_focus() == this) {
			gui->focus_something();
		}
	}
}

TGUI_Widget *TGUI_Widget::get_parent()
{
	return parent;
}

std::vector<TGUI_Widget *> &TGUI_Widget::get_children()
{
	return children;
}

int TGUI_Widget::get_x()
{
	return calculated_x;
}

int TGUI_Widget::get_y()
{
	return calculated_y;
}

int TGUI_Widget::get_width()
{
	if (calculated_w < 0) {
		return w;
	}
	return calculated_w;
}

int TGUI_Widget::get_height()
{
	if (calculated_h < 0) {
		return h;
	}
	return calculated_h;
}

int TGUI_Widget::get_padding_left()
{
	return calculated_padding_left;
}

int TGUI_Widget::get_padding_right()
{
	return calculated_padding_right;
}

int TGUI_Widget::get_padding_top()
{
	return calculated_padding_top;
}

int TGUI_Widget::get_padding_bottom()
{
	return calculated_padding_bottom;
}

int TGUI_Widget::get_offset_x()
{
	return offset_x;
}

int TGUI_Widget::get_offset_y()
{
	return offset_y;
}

void TGUI_Widget::init()
{
	gui = 0;
	parent = 0;
	use_percent_padding_left = false;
	use_percent_padding_right = false;
	use_percent_padding_top = false;
	use_percent_padding_bottom = false;
	padding_left = 0;
	padding_right = 0;
	padding_top = 0;
	padding_bottom = 0;
	float_left = false;
	float_right = false;
	float_bottom = false;
	centre_x = false;
	centre_y = false;
	clear_float_x = false;
	clear_float_y = false;
	break_line = false;
	accepts_focus = false;
	use_relative_position = false;
	offset_x = 0;
	offset_y = 0;
	left_widget = 0;
	right_widget = 0;
	up_widget = 0;
	down_widget = 0;
	calculated_x = -1;
	calculated_y = -1;
	calculated_w = -1;
	calculated_h = -1;
}

int TGUI_Widget::get_right_pos()
{
	if (float_left == false && float_right == false && centre_x == false) {
		return 0;
	}
	if (float_left) {
		return 0;
	}
	int parent_width;
	tgui_get_size(parent->parent, parent, &parent_width, 0, 0, 0, 0, 0);
	int width;
	tgui_get_size(parent, this, &width, 0, 0, 0, 0, 0);
	width += get_padding_left() + get_padding_right();
	int right = 0;
	int centre_this = 0;
	int centre_total[3] = { 0 }; // top middle bottom
	bool found = false;
	bool finalized = false;
	for (size_t i = 0; i < parent->children.size(); i++) {
		TGUI_Widget *d = parent->children[i];
		if (d->clear_float_x) {
			if (found) {
				finalized = true;
			}
			else {
				right = 0;
				if (d->centre_y) {
					centre_total[1] = 0;
					if (centre_y) {
						centre_this = 0;
					}
				}
				else if (d->float_bottom) {
					centre_total[2] = 0;
					if (float_bottom) {
						centre_this = 0;
					}
				}
				else {
					centre_total[0] = 0;
					if (centre_y == false && float_bottom == false) {
						centre_this = 0;
					}
				}
			}
		}
		if (d == this) {
			found = true;
			if (float_right) {
				break;
			}
			else {
				if (centre_y) {
					centre_this = centre_total[1];
				}
				else if (float_bottom) {
					centre_this = centre_total[2];
				}
				else {
					centre_this = centre_total[0];
				}
			}
		}
		if (d->float_right) {
			int w2;
			tgui_get_size(parent, d, &w2, 0, 0, 0, 0, 0);
			w2 += d->get_padding_left() + d->get_padding_right();
			right += w2;
		}
		else if (d->centre_x && !finalized) {
			if (d == this || !((d->centre_x && d->float_bottom) && (centre_x && float_bottom))) {
				int w2;
				tgui_get_size(parent, d, &w2, 0, 0, 0, 0, 0);
				w2 += d->get_padding_left() + d->get_padding_right();
				if (d->centre_y) {
					centre_total[1] += w2;
				}
				else if (d->float_bottom) {
					centre_total[2] += w2;
				}
				else if (!d->centre_y) {
					centre_total[0] += w2;
				}
			}
		}
	}

	if (centre_x) {
		parent_width += parent->get_padding_left() + parent->get_padding_right();
		int parent_centre = parent->get_padding_right() + (parent_width - parent->get_padding_left() - parent->get_padding_right()) / 2;
		int centre_total_this;
		if (centre_y) {
			centre_total_this = centre_total[1];
		}
		else if (float_bottom) {
			centre_total_this = centre_total[2];
		}
		else {
			centre_total_this = centre_total[0];
		}
		int widget_centre = centre_total_this / 2;
		int offset = parent->get_padding_left();
		return parent_width - parent_centre - widget_centre - offset + centre_this;
	}

	return parent_width - (right + width);
}

int TGUI_Widget::get_bottom_pos()
{
	if (float_bottom == false && centre_y == false) {
		return 0;
	}
	int parent_height;
	tgui_get_size(parent->parent, parent, 0, &parent_height, 0, 0, 0, 0);
	int height;
	tgui_get_size(parent, this, 0, &height, 0, 0, 0, 0);
	height += get_padding_top() + get_padding_bottom();
	if (centre_y) {
		parent_height += parent->get_padding_top() + parent->get_padding_bottom();
		int parent_centre = parent->get_padding_top() + (parent_height - parent->get_padding_top()) / 2;
		int widget_centre = height / 2;
		int offset = parent->get_padding_top();
		return parent_height - parent_centre - widget_centre - offset;
	}
	int bottom = 0;
	for (size_t i = 0; i < parent->children.size(); i++) {
		TGUI_Widget *d = parent->children[i];
		if (d->clear_float_y) {
			bottom = 0;
		}
		if (d == this) {
			break;
		}
		if (d->float_bottom) {
			int h2;
			tgui_get_size(parent, d, 0, &h2, 0, 0, 0, 0);
			h2 += d->get_padding_top() + d->get_padding_bottom();
			bottom += h2;
		}
	}

	return parent_height - (bottom + height);
}

void tgui_get_size(TGUI_Widget *parent, TGUI_Widget *widget, int *width, int *height, int *pad_l, int *pad_r, int *pad_t, int *pad_b)
{
	int parent_w, parent_h;

	if (parent == 0) {
		parent_w = widget->gui->w;
		parent_h = widget->gui->h;
		if (width) {
			int padding = 0;
			if (widget->use_percent_padding_left) {
				padding += int(parent_w * widget->percent_padding_left);
			}
			else {
				padding += widget->padding_left;
			}
			if (widget->use_percent_padding_right) {
				padding += int(parent_w * widget->percent_padding_right);
			}
			else {
				padding += widget->padding_right;
			}
			*width = widget->gui->w - padding;
		}
		if (height) {
			int padding = 0;
			if (widget->use_percent_padding_top) {
				padding += int(parent_h * widget->percent_padding_top);
			}
			else {
				padding += widget->padding_top;
			}
			if (widget->use_percent_padding_bottom) {
				padding += int(parent_h * widget->percent_padding_bottom);
			}
			else {
				padding += widget->padding_bottom;
			}
			*height = widget->gui->h - padding;
		}
	}
	else {
		int w = parent_w = parent->get_width();
		int h = parent_h = parent->get_height();
		if (width) {
			if (widget->calculated_w >= 0) {
				*width = widget->calculated_w;
			}
			else if (widget->percent_x) {
				if (widget->percent_w < 0) {
					int total_w = 0;
					bool found_widget = false;
					for (size_t i = 0; i < parent->children.size(); i++) {
						TGUI_Widget *d = parent->children[i];
						if (d == widget) {
							found_widget = true;
						}
						int this_w = 0;
						if (d != widget && (d->percent_x == false || d->percent_w >= 0)) {
							tgui_get_size(parent, d, &this_w, 0, 0, 0, 0, 0);
						}
						if (d->float_right == false && d->centre_x == false && d->use_relative_position == false) {
							total_w += this_w;
						}
						if (total_w + this_w > w) {
							if (found_widget) {
								break;
							}
							total_w = 0;
						}
					}
					int remainder = w - total_w;
					if (remainder > 0) {
						*width = int(remainder * -widget->percent_w);
					}
					else {
						*width = 0;
					}
				}
				else {
					int padding = 0;
					if (widget->use_percent_padding_left) {
						padding += int(parent_w * widget->percent_padding_left);
					}
					else {
						padding += widget->padding_left;
					}
					if (widget->use_percent_padding_right) {
						padding += int(parent_w * widget->percent_padding_right);
					}
					else {
						padding += widget->padding_right;
					}
					*width = int(w * widget->percent_w) - padding;
				}
			}
			else {
				*width = widget->w;
			}
		}
		if (height) {
			if (widget->calculated_h >= 0) {
				*height = widget->calculated_h;
			}
			else if (widget->percent_y) {
				if (widget->percent_h < 0) {
					int total_w = 0;
					int max_h = 0;
					bool found_widget = false;
					for (size_t i = 0; i < parent->children.size(); i++) {
						TGUI_Widget *d = parent->children[i];
						if (d == widget) {
							found_widget = true;
						}
						int this_w;
						int this_h;
						if (d == widget) {
							this_w = 0;
							this_h = 0;
						}
						else {
							tgui_get_size(parent, d, &this_w, &this_h, 0, 0, 0, 0);
							if (d->percent_y && d->percent_h < 0) {
								this_h = 0;
							}
						}
						if (total_w + this_w > w) {
							max_h = this_h;
						}
						else if (this_h > max_h) {
							max_h = this_h;
						}
						if (d->float_right == false && d->centre_x == false && d->use_relative_position == false) {
							total_w += this_w;
						}
						if (total_w + this_w >= w) {
							if (found_widget) {
								break;
							}
							if (total_w + this_w > w) {
								total_w = this_w;
							}
							else {
								max_h = 0;
								total_w = 0;
							}
						}
					}
					int remainder = h - max_h;
					if (remainder > 0) {
						*height = int(remainder * -widget->percent_h);
					}
					else {
						*height = 0;
					}
				}
				else {
					int padding = 0;
					if (widget->use_percent_padding_top) {
						padding += int(parent_h * widget->percent_padding_top);
					}
					else {
						padding += widget->padding_top;
					}
					if (widget->use_percent_padding_bottom) {
						padding += int(parent_h * widget->percent_padding_bottom);
					}
					else {
						padding += widget->padding_bottom;
					}
					*height = int(h * widget->percent_h) - padding;
				}
			}
			else {
				*height = widget->h;
			}
		}
	}

	if (pad_l) {
		if (widget->use_percent_padding_left) {
			*pad_l = int(parent_w * widget->percent_padding_left);

		}
		else {
			*pad_l = widget->padding_left;
		}
	}
	if (pad_r) {
		if (widget->use_percent_padding_right) {
			*pad_r = int(parent_w * widget->percent_padding_right);

		}
		else {
			*pad_r = widget->padding_right;
		}
	}
	if (pad_t) {
		if (widget->use_percent_padding_top) {
			*pad_t = int(parent_h * widget->percent_padding_top);

		}
		else {
			*pad_t = widget->padding_top;
		}
	}
	if (pad_b) {
		if (widget->use_percent_padding_bottom) {
			*pad_b = int(parent_h * widget->percent_padding_bottom);

		}
		else {
			*pad_b = widget->padding_bottom;
		}
	}
}

TGUI_Event tgui_get_relative_event(TGUI_Widget *widget, TGUI_Event *event)
{
	TGUI_Event new_event = *event;

	if (new_event.type == TGUI_MOUSE_DOWN || new_event.type == TGUI_MOUSE_UP || new_event.type == TGUI_MOUSE_AXIS) {
		new_event.mouse.x -= widget->get_x();
		new_event.mouse.y -= widget->get_y();

		// -1 if out of bounds on any dimension
		if (new_event.mouse.x < 0 || new_event.mouse.y < 0 || new_event.mouse.x >= widget->get_width() || new_event.mouse.y >= widget->get_height()) {
			new_event.mouse.x = new_event.mouse.y = -1;
		}
	}

	return new_event;
}
