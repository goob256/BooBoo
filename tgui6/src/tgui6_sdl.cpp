#include <cstring>

#include "tgui6/tgui6.h"
#include "tgui6/tgui6_sdl.h"

TGUI_Event tgui_sdl_convert_event(SDL_Event *sdl_event)
{
	TGUI_Event event;

	switch (sdl_event->type) {
		case SDL_QUIT:
			event.type = TGUI_QUIT;
			break;
		case SDL_KEYDOWN:
			event.type = TGUI_KEY_DOWN;
			event.keyboard.code = sdl_event->key.keysym.sym;
			event.keyboard.is_repeat = sdl_event->key.repeat != 0;
			event.keyboard.simulated = false;
			break;
		case SDL_KEYUP:
			event.type = TGUI_KEY_UP;
			event.keyboard.code = sdl_event->key.keysym.sym;
			event.keyboard.is_repeat = sdl_event->key.repeat != 0;
			event.keyboard.simulated = false;
			break;
		case SDL_CONTROLLERBUTTONDOWN:
			event.type = TGUI_JOY_DOWN;
			event.joystick.id = sdl_event->cbutton.which;
			event.joystick.button = sdl_event->cbutton.button;
			event.joystick.axis = -1;
			event.joystick.value = 0.0f;
			event.joystick.is_repeat = false;
			break;
		case SDL_CONTROLLERBUTTONUP:
			event.type = TGUI_JOY_UP;
			event.joystick.id = sdl_event->cbutton.which;
			event.joystick.button = sdl_event->cbutton.button;
			event.joystick.axis = -1;
			event.joystick.value = 0.0f;
			event.joystick.is_repeat = false;
			break;
		case SDL_CONTROLLERAXISMOTION:
			event.type = TGUI_JOY_AXIS;
			event.joystick.id = sdl_event->caxis.which;
			event.joystick.button = -1;
			event.joystick.axis = sdl_event->caxis.axis;
			event.joystick.value = TGUI6_NORMALISE_JOY_AXIS(sdl_event->caxis.value);
			event.joystick.is_repeat = false;
			break;
#ifndef TVOS
		case SDL_MOUSEBUTTONDOWN:
			if (sdl_event->button.which != SDL_TOUCH_MOUSEID) {
				event.type = TGUI_MOUSE_DOWN;
				event.mouse.button = sdl_event->button.button;
				event.mouse.x = (float)sdl_event->button.x;
				event.mouse.y = (float)sdl_event->button.y;
				event.mouse.normalised = false;
				event.mouse.is_touch = false;
				event.mouse.is_repeat = false;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (sdl_event->button.which != SDL_TOUCH_MOUSEID) {
				event.type = TGUI_MOUSE_UP;
				event.mouse.button = sdl_event->button.button;
				event.mouse.x = (float)sdl_event->button.x;
				event.mouse.y = (float)sdl_event->button.y;
				event.mouse.normalised = false;
				event.mouse.is_touch = false;
				event.mouse.is_repeat = false;
			}
			break;
		case SDL_MOUSEMOTION:
			if (sdl_event->motion.which != SDL_TOUCH_MOUSEID) {
				event.type = TGUI_MOUSE_AXIS;
				event.mouse.button = sdl_event->button.button;
				event.mouse.x = (float)sdl_event->motion.x;
				event.mouse.y = (float)sdl_event->motion.y;
				event.mouse.normalised = false;
				event.mouse.is_touch = false;
				event.mouse.is_repeat = false;
			}
			break;
		case SDL_MOUSEWHEEL:
			event.type = TGUI_MOUSE_WHEEL;
			event.mouse.button = -1;
			event.mouse.x = (float)sdl_event->wheel.x;
			event.mouse.y = (float)sdl_event->wheel.y;
			event.mouse.normalised = false;
			break;
		case SDL_FINGERDOWN:
			event.type = TGUI_MOUSE_DOWN;
			event.mouse.button = SDL_BUTTON_LEFT;
			event.mouse.x = (float)sdl_event->tfinger.x;
			event.mouse.y = (float)sdl_event->tfinger.y;
			event.mouse.normalised = true;
			event.mouse.is_touch = true;
			event.mouse.finger = sdl_event->tfinger.fingerId;
			event.mouse.is_repeat = false;
			break;
		case SDL_FINGERUP:
			event.type = TGUI_MOUSE_UP;
			event.mouse.button = SDL_BUTTON_LEFT;
			event.mouse.x = (float)sdl_event->tfinger.x;
			event.mouse.y = (float)sdl_event->tfinger.y;
			event.mouse.normalised = true;
			event.mouse.is_touch = true;
			event.mouse.finger = sdl_event->tfinger.fingerId;
			event.mouse.is_repeat = false;
			break;
		case SDL_FINGERMOTION:
			event.type = TGUI_MOUSE_AXIS;
			event.mouse.button = SDL_BUTTON_LEFT;
			event.mouse.x = (float)sdl_event->tfinger.x;
			event.mouse.y = (float)sdl_event->tfinger.y;
			event.mouse.normalised = true;
			event.mouse.is_touch = true;
			event.mouse.finger = sdl_event->tfinger.fingerId;
			event.mouse.is_repeat = false;
			break;
#endif
		case SDL_TEXTINPUT:
			event.type = TGUI_TEXT;
			strcpy(event.text.text, sdl_event->text.text);
			break;
		default:
			event.type = TGUI_UNKNOWN;
			break;
	}

	return event;
}
