#ifndef NOO_INPUT_H
#define NOO_INPUT_H

#include "shim4/main.h"
#include "shim4/steamworks.h"

namespace noo {

namespace input {

struct SHIM4_EXPORT Focus_Event : public TGUI_Event {
	TGUI_Event_Type orig_type;
	union {
		TGUI_Event::TGUI_Event_Keyboard orig_keyboard;
		TGUI_Event::TGUI_Event_Joystick orig_joystick;
	} u;

	virtual ~Focus_Event();
};

bool start();
void reset();
void end();
void update();
void handle_event(TGUI_Event *event);

bool SHIM4_EXPORT convert_to_focus_event(TGUI_Event *event, Focus_Event *focus);
void SHIM4_EXPORT convert_focus_to_original(TGUI_Event *event);
void SHIM4_EXPORT rumble(Uint32 length);
bool SHIM4_EXPORT is_joystick_connected();
std::string SHIM4_EXPORT get_joystick_button_name(int button);
std::string SHIM4_EXPORT get_joystick_button_colour_code(int button);
void SHIM4_EXPORT drop_repeats(bool joystick = true, bool mouse = true);
int SHIM4_EXPORT get_num_joysticks();
SDL_JoystickID SHIM4_EXPORT get_controller_id(int index);
SDL_Joystick SHIM4_EXPORT *get_sdl_joystick(SDL_JoystickID id);
SDL_GameController SHIM4_EXPORT *get_sdl_gamecontroller(SDL_JoystickID id);

#ifdef STEAMWORKS
ControllerHandle_t SHIM4_EXPORT get_controller_handle();
#endif

bool SHIM4_EXPORT system_has_touchscreen();
bool SHIM4_EXPORT system_has_keyboard();

} // End namespace input

} // End namespace noo

#endif // NOO_INPUT_H
