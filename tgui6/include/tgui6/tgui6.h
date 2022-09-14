#ifndef TGUI6_H
#define TGUI6_H

#ifdef _WIN32
#pragma warning(disable : 4251)
#ifdef TGUI6_STATIC
#define TGUI_EXPORT
#else
#ifdef TGUI6_LIB_BUILD
#define TGUI_EXPORT __declspec(dllexport)
#else
#define TGUI_EXPORT __declspec(dllimport)
#endif
#endif
#else
#define TGUI_EXPORT
#endif

#include <cassert>

#include <vector>

#if defined __APPLE__
#include <SDL2/SDL.h>
#elif defined STEAMWORKS && defined __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#define TGUI6_NORMALISE_JOY_AXIS(value) ((float(value) + 32768.0f) / 65535.0f * 2.0f - 1.0f)

enum TGUI_Event_Type {
	TGUI_UNKNOWN,
	TGUI_KEY_DOWN,
	TGUI_KEY_UP,
	TGUI_MOUSE_DOWN,
	TGUI_MOUSE_UP,
	TGUI_MOUSE_AXIS,
	TGUI_JOY_DOWN,
	TGUI_JOY_UP,
	TGUI_JOY_AXIS,
	TGUI_FOCUS,
	TGUI_MOUSE_WHEEL, // mouse.x=horizontal (positive right, negative left), mouse.y=vertical (positive forward, negative backward)
	TGUI_TICK, // timer tick, 1 logic frame
	TGUI_QUIT, // titlebar X pressed on desktop
	TGUI_TEXT // text like typing on keyboard
};

enum TGUI_Focus_Type {
	TGUI_FOCUS_UNKNOWN,
	TGUI_FOCUS_LEFT,
	TGUI_FOCUS_RIGHT,
	TGUI_FOCUS_UP,
	TGUI_FOCUS_DOWN
};

class TGUI_EXPORT TGUI_Event {
public:
	TGUI_Event_Type type;
	struct TGUI_Event_Keyboard {
		int code;
		bool is_repeat;
		bool simulated;
	} keyboard;
	struct TGUI_Event_Mouse {
		int button;
		float x;
		float y;
		bool normalised; // if true, x/y are between 0 and 1 and not in screen coordinates
		bool is_touch;
		SDL_FingerID finger;
		bool is_repeat;
	} mouse;
	struct TGUI_Event_Joystick {
		SDL_JoystickID id; // this would need changing if anything besides SDL is supported
		int button;
		int axis;
		float value; // -1.0 -> 1.0
		bool is_repeat;
	} joystick;
	struct TGUI_Event_Focus {
		TGUI_Focus_Type type;
	} focus;
	struct TGUI_Event_Text {
		char text[32]; // UTF-8 text
	} text;

	TGUI_Event();
	TGUI_Event(TGUI_Event const &x);
	TGUI_Event &operator=(const TGUI_Event &x);
	virtual ~TGUI_Event();
};

class TGUI;
class TGUI_Widget;

TGUI_EXPORT void tgui_get_size(TGUI_Widget *parent, TGUI_Widget *widget, int *width, int *height, int *pad_l, int *pad_r, int *pad_t, int *pad_b);
TGUI_EXPORT TGUI_Event tgui_get_relative_event(TGUI_Widget *widget, TGUI_Event *event);

// a GUI hierarchy
class TGUI {
	friend TGUI_EXPORT void tgui_get_size(TGUI_Widget *parent, TGUI_Widget *widget, int *width, int *height, int *pad_l, int *pad_r, int *pad_t, int *pad_b);

public:
	static void set_focus_sloppiness(int sloppiness); // 0-2, default is 2, less means stricter rules to change focus

	TGUI_EXPORT TGUI(TGUI_Widget *main_widget, int w, int h);
	TGUI_EXPORT ~TGUI();

	TGUI_EXPORT void layout();
	TGUI_EXPORT void resize(int w, int h);
	TGUI_EXPORT void draw();
	TGUI_EXPORT void handle_event(TGUI_Event *event);

	TGUI_EXPORT void set_focus(TGUI_Widget *widget);
	TGUI_EXPORT void focus_something();
	TGUI_EXPORT void set_offset(int offset_x, int offset_y);

	TGUI_EXPORT TGUI_Widget *get_main_widget();
	TGUI_EXPORT TGUI_Widget *get_focus();
	TGUI_EXPORT TGUI_Widget *get_event_owner(TGUI_Event *event);
	TGUI_EXPORT int get_width();
	TGUI_EXPORT int get_height();

private:
	TGUI_EXPORT void destroy(TGUI_Widget *widget);
	TGUI_EXPORT void reset_size(TGUI_Widget *widget);
	TGUI_EXPORT void set_sizes(TGUI_Widget *widget);
	TGUI_EXPORT int set_positions(TGUI_Widget *widget, int x, int y);
	TGUI_EXPORT void draw(TGUI_Widget *widget);
	TGUI_EXPORT TGUI_Widget *get_event_owner(TGUI_Event *event, TGUI_Widget *widget);
	TGUI_EXPORT void handle_event(TGUI_Event *event, TGUI_Widget *widget);
	TGUI_EXPORT bool focus_something(TGUI_Widget *widget);
	TGUI_EXPORT void focus_distance(TGUI_Widget *start, TGUI_Widget *widget, int dir_x, int dir_y, int &score, int &grade);
	TGUI_EXPORT void find_focus(TGUI_Widget *start, TGUI_Widget *&current_best, TGUI_Widget *widget, int dir_x, int dir_y, int &best_score, int &best_grade);

	static int focus_sloppiness;

	TGUI_Widget *main_widget;
	TGUI_Widget *focus;
	int w, h;
	int offset_x, offset_y;
};

class TGUI_Widget;

class TGUI_EXPORT TGUI_Widget {
	friend TGUI_EXPORT void tgui_get_size(TGUI_Widget *parent, TGUI_Widget *widget, int *width, int *height, int *pad_l, int *pad_r, int *pad_t, int *pad_b);
	friend class TGUI_EXPORT TGUI;

public:
	enum Fit {
		FIT_X,
		FIT_Y
	};

	/* Percentage sizes can be negative or positive: positive means % of parent size,
	 * negative means % of whatever's left after fixed and positive percentage sized
	 * widgets.
	 */
	TGUI_Widget(int w, int h);
	TGUI_Widget(float percent_w, float percent_h);
	TGUI_Widget(int w, float percent_h);
	TGUI_Widget(float percent_w, int h);

	TGUI_Widget(Fit fit, int other);
	TGUI_Widget(Fit fit, float percent_other);
	TGUI_Widget(); // fit both

	virtual ~TGUI_Widget();

	virtual void draw();
	virtual void end_draw();
	virtual void handle_event(TGUI_Event *event);
	virtual void resize();

	void set_parent(TGUI_Widget *widget);
	void set_float_left(bool float_left);
	void set_float_right(bool float_right);
	void set_float_bottom(bool float_bottom);
	void set_centre_x(bool centre_x);
	void set_centre_y(bool centre_y);
	void set_clear_float_x(bool clear_float_x);
	void set_clear_float_y(bool clear_float_y);
	void set_break_line(bool break_line);
	void set_accepts_focus(bool accepts_focus);

	void set_width(int width);
	void set_width(float percent_width);
	void set_height(int height);
	void set_height(float percent_height);

	void set_padding_left(int padding);
	void set_padding_left(float percent_padding);
	void set_padding_right(int padding);
	void set_padding_right(float percent_padding);
	void set_padding_top(int padding);
	void set_padding_top(float percent_padding);
	void set_padding_bottom(int padding);
	void set_padding_bottom(float percent_padding);
	void set_padding(int padding);
	void set_padding(float percent_padding);

	void set_relative_position(int relative_x, int relative_y);

	void set_offset_x(int offset_x);
	void set_offset_y(int offset_y);

	void set_left_widget(TGUI_Widget *widget);
	void set_right_widget(TGUI_Widget *widget);
	void set_up_widget(TGUI_Widget *widget);
	void set_down_widget(TGUI_Widget *widget);

	TGUI_Widget *get_parent();
	std::vector<TGUI_Widget *> &get_children();

	int get_x();
	int get_y();
	int get_width();
	int get_height();
	bool get_float_left();
	bool get_float_right();
	bool get_float_bottom();
	bool get_centre_x();
	bool get_centre_y();
	bool get_clear_float_x();
	bool get_clear_float_y();
	bool get_break_line();
	bool get_accepts_focus();
	int get_padding_left();
	int get_padding_right();
	int get_padding_top();
	int get_padding_bottom();
	int get_offset_x();
	int get_offset_y();

protected:
	void init();
	int get_right_pos();
	int get_bottom_pos();

	TGUI *gui;
	TGUI_Widget *parent;
	bool fit_x, fit_y;
	bool fitted_x, fitted_y; // done being fitted (for now)
	bool percent_x, percent_y;
	float percent_w, percent_h;
	int w, h;
	std::vector<TGUI_Widget *> children;
	bool use_percent_padding_left, use_percent_padding_right, use_percent_padding_top, use_percent_padding_bottom;
	float percent_padding_left, percent_padding_right, percent_padding_top, percent_padding_bottom;
	int padding_left, padding_right, padding_top, padding_bottom;
	bool float_left;
	bool float_right;
	bool float_bottom;
	bool centre_x;
	bool centre_y;
	bool accepts_focus;

	bool clear_float_x;
	bool clear_float_y;
	bool break_line;

	bool use_relative_position;
	int relative_x, relative_y;

	int offset_x, offset_y;

	int calculated_x, calculated_y;
	int calculated_w, calculated_h;
	int calculated_padding_left, calculated_padding_right, calculated_padding_top, calculated_padding_bottom;

	TGUI_Widget *left_widget;
	TGUI_Widget *right_widget;
	TGUI_Widget *up_widget;
	TGUI_Widget *down_widget;
};

#endif // TGUI6_H
