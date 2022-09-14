#ifndef NOO_DEVSETTINGS_H
#define NOO_DEVSETTINGS_H

#include "shim4/main.h"
#include "shim4/gui.h"

namespace noo {

namespace gui {

class DevSettings_Label;
class DevSettings_List;

class DevSettings_GUI : public GUI
{
public:
	DevSettings_GUI();
	virtual ~DevSettings_GUI();

	bool is_editing();
	void update();

private:
	DevSettings_List *list;
};

class DevSettings_NumGetter_GUI : public GUI
{
public:
	struct Callback_Data {
		std::string text;
		void *userdata;
	};

	DevSettings_NumGetter_GUI(std::string text, std::string initial, bool decimals_allowed, util::Callback callback = 0, void *callback_data = 0);
	virtual ~DevSettings_NumGetter_GUI();

	void handle_event(TGUI_Event *event);

private:
	std::string get_character_text();

	DevSettings_Label *header;
	DevSettings_Label *number;
	DevSettings_Label *character;

	util::Callback callback;
	void *callback_data;

	int _character;
	bool decimals_allowed;
};

} // End namespace gui

} // End namespace noo

#endif // NOO_DEVSETTINGS_H
