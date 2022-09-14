#ifndef NOO_WIDGETS_H
#define NOO_WIDGETS_H

#include "shim4/main.h"
#include "shim4/json.h"

namespace noo {

namespace gui {

class SHIM4_EXPORT Widget : public TGUI_Widget
{
public:
	Widget(int w, int h);
	Widget(float percent_w, float percent_h);
	Widget(int w, float percent_h);
	Widget(float percent_w, int h);
	Widget(TGUI_Widget::Fit fit, int other);
	Widget(TGUI_Widget::Fit fit, float percent_other);
	Widget(); // Fit both
	virtual ~Widget();

	void draw();

	SDL_Colour bg_colour;

protected:
	void start();
};

class SHIM4_EXPORT DevSettings_List : public Widget
{
public:
	DevSettings_List();
	virtual ~DevSettings_List();

	void draw();
	void handle_event(TGUI_Event *event);

	bool is_editing();

	void set_value(std::string text);

	bool is_done();

private:
	void set_nr();
	void draw_text_clamped(SDL_Colour colour, std::string text, util::Point<int> pos, int max_width);
	void draw_text_scroll(SDL_Colour colour, std::string text, util::Point<int> pos, int max_width);
	void save_edit();
	void draw_edit(std::string value, util::Point<int> pos, int max_width);
	void insert_text(char *text);
	void set_edit_offset();

	int top;
	int selected;
	bool editing;
	util::JSON::Node *node;
	std::string tmpval; // during editing
	int nr;
	int row_h;
	Uint32 change_time;
	int edit_offset;
	int cursor_pos;
	int val_width;

	bool done;
};

class SHIM4_EXPORT DevSettings_Label : public Widget
{
public:
	DevSettings_Label(std::string text);
	virtual ~DevSettings_Label();

	void draw();

	void set_text(std::string text);
	std::string get_text();

private:
	std::string text;
};

} // End namespace gui

} // End namespace noo

#endif // NOO_WIDGETS_H
