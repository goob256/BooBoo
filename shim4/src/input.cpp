#include "shim4/gfx.h"
#include "shim4/input.h"
#include "shim4/shim.h"
#include "shim4/util.h"

#ifdef STEAM_INPUT
#include "shim4/steamworks.h"
ControllerHandle_t all_controllers[STEAM_CONTROLLER_MAX_COUNT];
#endif

#ifdef __APPLE__
#define USE_CONSTANT_RUMBLE 1
#else
#define USE_CONSTANT_RUMBLE 0
#endif

#ifdef ANDROID
#include <jni.h>
#endif

#define REPEAT_VEC std::vector<Joy_Repeat>

#if defined ANDROID || defined TVOS
const Uint32 JOYSTICK_SUBSYSTEMS = SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC;
#endif

#ifdef IOS
#import <AudioToolbox/AudioToolbox.h>
#endif

using namespace noo;

#include "shim4/internal/gfx.h"
#include "shim4/internal/shim.h"

struct Joy_Repeat {
	bool is_button;
	int button;
	int axis;
	int value;
	Uint32 initial_press_time;
	bool down;
	bool repeated;
};

struct Mouse_Button_Repeat {
	bool is_touch;
	int button;
	SDL_FingerID finger;
	util::Point<float> down_pos;
	Uint32 initial_press_time;
	bool down;
};

enum Joystick_Type {
	XBOX,
	NINTENDO,
	PLAYSTATION,
	XBONE
#ifdef IOS
	,IOS_JOY
#endif
};

struct Joystick {
#ifdef STEAM_INPUT
	ControllerHandle_t handle;
#endif
	SDL_Haptic *haptic;
#if USE_CONSTANT_RUMBLE
	SDL_HapticEffect haptic_effect;
	int haptic_effect_id;
#endif
	SDL_GameController *gc;
	SDL_Joystick *joy;
	SDL_JoystickID id;
	Joystick_Type type;
	REPEAT_VEC joystick_repeats;
};

static std::vector<Joystick> joysticks;
static int num_joysticks;
static std::vector<Mouse_Button_Repeat> mouse_button_repeats;

static Joystick *find_joystick(SDL_JoystickID id)
{
	for (size_t i = 0; i < joysticks.size(); i++) {
		if (joysticks[i].id == id) {
			return &joysticks[i];
		}
	}

	return 0;
}

static int find_joy_repeat(bool is_button, int n, Joystick *js)
{
	REPEAT_VEC &joystick_repeats = js->joystick_repeats;

	for (size_t i = 0; i < joystick_repeats.size(); i++) {
		if (is_button && joystick_repeats[i].is_button && joystick_repeats[i].button == n) {
			return (int)i;
		}
		if (!is_button && !joystick_repeats[i].is_button && joystick_repeats[i].axis == n) {
			return (int)i;
		}
	}

	return -1;
}

static int find_mouse_button_repeat(bool is_touch, int button, SDL_FingerID finger)
{
	for (size_t i = 0; i < mouse_button_repeats.size(); i++) {
		if (mouse_button_repeats[i].is_touch == is_touch) {
			if (is_touch && mouse_button_repeats[i].finger == finger) {
				return (int)i;
			}
			else if (is_touch == false && mouse_button_repeats[i].button == button) {
				return (int)i;
			}
		}
	}

	return -1;
}

static bool check_joystick_repeat(Joy_Repeat &jr)
{
	// these define the repeat rate in thousands of a second
	int initial_delay = 500;
	int repeat_delay = 50;
	Uint32 diff = SDL_GetTicks() - jr.initial_press_time;
	if ((int)diff > initial_delay) {
		diff -= initial_delay;
		int elapsed = diff;
		int mod = elapsed % repeat_delay;
		if (mod < repeat_delay / 2) {
			// down
			if (jr.down == false) {
				jr.down = true;
				return true;
			}
		}
		else {
			// up
			jr.down = false;
		}
	}

	return false;
}

static bool check_mouse_button_repeat(Mouse_Button_Repeat &mr)
{
	// these define the repeat rate in thousands of a second
	int initial_delay = 500;
	int repeat_delay = 50;
	Uint32 diff = SDL_GetTicks() - mr.initial_press_time;
	if ((int)diff > initial_delay) {
		diff -= initial_delay;
		int elapsed = diff;
		int mod = elapsed % repeat_delay;
		if (mod < repeat_delay / 2) {
			// down
			if (mr.down == false) {
				mr.down = true;
				return true;
			}
		}
		else {
			// up
			mr.down = false;
		}
	}

	return false;
}

#if 0
void SDL_GetJoystickGUIDInfo(SDL_JoystickGUID guid, Uint16 *vendor, Uint16 *product, Uint16 *version)
{
    Uint16 *guid16 = (Uint16 *)guid.data;

    /* If the GUID fits the form of BUS 0000 VENDOR 0000 PRODUCT 0000, return the data */
    if (/* guid16[0] is device bus type */
        guid16[1] == 0x0000 &&
        /* guid16[2] is vendor ID */
        guid16[3] == 0x0000 &&
        /* guid16[4] is product ID */
        guid16[5] == 0x0000
        /* guid16[6] is product version */
   ) {
        if (vendor) {
            *vendor = guid16[2];
        }
        if (product) {
            *product = guid16[4];
        }
        if (version) {
            *version = guid16[6];
        }
    } else {
        if (vendor) {
            *vendor = 0;
        }
        if (product) {
            *product = 0;
        }
        if (version) {
            *version = 0;
        }
    }
}
#endif

static void add_haptics(Joystick *j)
{
	if (SDL_JoystickIsHaptic(j->joy)) {
		util::infomsg("Joystick has haptics.\n");
		j->haptic = SDL_HapticOpenFromJoystick(j->joy);
		if (j->haptic == 0) {
			util::infomsg("Haptic init failed: %s.\n", SDL_GetError());
		}
		else {
#if USE_CONSTANT_RUMBLE
			memset(&j->haptic_effect, 0, sizeof(j->haptic_effect));
			j->haptic_effect.type = SDL_HAPTIC_CONSTANT;
			j->haptic_effect.constant.level = 0x7fff;
			j->haptic_effect.constant.length = 1000;
			j->haptic_effect_id = SDL_HapticNewEffect(j->haptic, &j->haptic_effect);
			if (j->haptic_effect_id < 0) {
				util::infomsg("Couldn't create constant haptic effect.\n");
			}
#else
			if (SDL_HapticRumbleInit(j->haptic) != 0) {
				util::infomsg("Can't init rumble effect: %s\n", SDL_GetError());
			}
#endif
			if (SDL_HapticSetGain(j->haptic, 100)) {
				util::infomsg("Can't set haptic gain: %s\n", SDL_GetError());
			}
		}
	}
	else {
		util::infomsg("Joystick does not have haptics\n");
		j->haptic = 0;
	}
}

static void check_joysticks()
{
	Joystick j;

#ifdef STEAM_INPUT
	if (shim::steam_init_failed == false) {
		ControllerHandle_t controllers[STEAM_CONTROLLER_MAX_COUNT];
		int nj = SteamController()->GetConnectedControllers(controllers);
		bool same = nj == num_joysticks;
		if (same) {
			for (int i = 0; i < nj; i++) {
				if (controllers[i] != all_controllers[i]) {
					same = false;
					break;
				}
			}
		}
		if (same == false) {
			if (nj == 0 && num_joysticks != 0) {
				if (shim::joystick_disconnect_callback) {
					shim::joystick_disconnect_callback();
				}
			}
			num_joysticks = nj;
			for (int i = 0; i < nj; i++) {
				all_controllers[i] = controllers[i];
			}
			for (size_t i = 0; i < joysticks.size(); i++) {
				SDL_GameControllerClose(joysticks[i].gc);
			}
			joysticks.clear();
			for (int i = 0; i < num_joysticks; i++) {
				/*
				j.joy = SDL_JoystickOpen(i);
				if (j.joy == NULL) {
					util::debugmsg("Couldn't open joystick: %s\n", SDL_GetError());
					continue;
				}
				j.id = SDL_JoystickInstanceID(j.joy);
				*/
				j.handle = controllers[i];
				j.gc = SDL_GameControllerOpen(i);
				j.joy = SDL_GameControllerGetJoystick(j.gc);
				j.id = SDL_JoystickInstanceID(j.joy);
				/*
				j.id = (SDL_JoystickID)j.handle;
				j.joy = SDL_JoystickFromInstanceID(j.id);
				*/
				ESteamInputType inputType = SteamController()->GetInputTypeForHandle(j.handle);
				switch ((int)inputType) {
					case k_ESteamInputType_XBoxOneController:
						j.type = XBONE;
						break;
					case k_ESteamInputType_PS3Controller:
					case k_ESteamInputType_PS4Controller:
						j.type = PLAYSTATION;
						break;
					case k_ESteamInputType_SwitchJoyConPair:
					case k_ESteamInputType_SwitchJoyConSingle:
					case k_ESteamInputType_SwitchProController:
						j.type = NINTENDO;
						break;
					default:
						j.type = XBOX;
						break;	
				}
				//add_haptics(&j);
				joysticks.push_back(j);
				// FIXME: support multiple joysticks
				break;
			}
			//gfx::show_mouse_cursor(joysticks.size() == 0);
		}
	}
	else
#endif
	{
		int nj = SDL_NumJoysticks();
		if (nj != num_joysticks) {
			if (nj == 0 && num_joysticks != 0) {
				if (shim::joystick_disconnect_callback) {
					shim::joystick_disconnect_callback();
				}
			}
			num_joysticks = nj;
			for (size_t i = 0; i < joysticks.size(); i++) {
				if (joysticks[i].haptic) {
					SDL_HapticClose(joysticks[i].haptic);
				}
				SDL_GameControllerClose(joysticks[i].gc);
			}
			joysticks.clear();
			for (int i = 0; i < num_joysticks; i++) {
				j.gc = SDL_GameControllerOpen(i);
				if (j.gc == NULL) {
					util::infomsg("Error opening game controller: %s\n", SDL_GetError());

					/*
					SDL_Joystick *joy = SDL_JoystickOpen(i);
					SDL_JoystickGUID guid;
					Uint16 vendor;
					Uint16 product;
					guid = SDL_JoystickGetGUID(joy);
					SDL_GetJoystickGUIDInfo(guid, &vendor, &product, NULL);
					char *c = (char *)&guid;
					printf("guid=");
					for (int j = 0; j < 16; j++) {
						printf("%02x", c[j]);
					}
					printf("\n");
					printf("vendor=%x product=%x\n", guid, vendor, product);
					SDL_JoystickClose(joy);
					*/

					continue;
				}
				std::string name = SDL_GameControllerName(j.gc);
				name = util::uppercase(name);
				if (name.find("PS2") != std::string::npos || name.find("PS3") != std::string::npos || name.find("PS4") != std::string::npos || name.find("PLAYSTATION") != std::string::npos || name.find("DUALSHOCK") != std::string::npos) {
					j.type = PLAYSTATION;
				}
				else if (name.find("NINTENDO") != std::string::npos || name.find("SWITCH") != std::string::npos) {
					j.type = NINTENDO;
				}
#ifdef IOS
				else if (name.find("XBOX") != std::string::npos) {
#else
				else if (name.find("XBOX ONE") != std::string::npos || name.find("X-BOX ONE") != std::string::npos) {
#endif
					j.type = XBONE;
				}
#ifdef IOS
				else {
					j.type = IOS_JOY;
				}
#else
				else {
					j.type = XBOX;
				}
#endif
				j.joy = SDL_GameControllerGetJoystick(j.gc);
				if (SDL_JoystickNumButtons(j.joy) < 5) {
					SDL_GameControllerClose(j.gc);
					continue;
				}
				else {
					j.id = SDL_JoystickInstanceID(j.joy);
					add_haptics(&j);
					joysticks.push_back(j);
				}
				// FIXME: support multiple joysticks
				//break;
			}
			//gfx::show_mouse_cursor(joysticks.size() == 0);
		}
	}
}

namespace noo {

namespace input {

bool start()
{
	int index;
	if ((index = util::check_args(shim::argc, shim::argv, "+joystick-activate-threshold") > 0)) {
		shim::joystick_activate_threshold = atof(shim::argv[index+1]);
	}
	else {
		shim::joystick_activate_threshold = 0.8f;
	}
	if ((index = util::check_args(shim::argc, shim::argv, "+joystick-deactivate-threshold") > 0)) {
		shim::joystick_deactivate_threshold = atof(shim::argv[index+1]);
	}
	else {
		shim::joystick_deactivate_threshold = 0.75f;
	}

	joysticks.clear();
	num_joysticks = 0;

#ifdef TVOS
	SDL_SetHint("SDL_HINT_TV_REMOTE_AS_JOYSTICK", "0");
	SDL_SetHint("SDL_HINT_ACCELEROMETER_AS_JOYSTICK", "0");
	SDL_SetHint("SDL_HINT_APPLE_TV_REMOTE_ALLOW_ROTATION", "0");
#endif


#if defined ANDROID || defined TVOS
	SDL_InitSubSystem(JOYSTICK_SUBSYSTEMS);
	SDL_GameControllerEventState(SDL_ENABLE);
#endif

 	check_joysticks();

	return true;
}

void reset()
{
	for (size_t i = 0; i < joysticks.size(); i++) {
		Joystick &j = joysticks[i];
#ifdef STEAM_INPUT
		if (shim::steam_init_failed)
#endif
			if (j.haptic) {
			       SDL_HapticClose(j.haptic);
			}
		if (j.gc) {
			SDL_GameControllerClose(j.gc);
		}
	}

	joysticks.clear();

	num_joysticks = 0;
}

void end()
{
	reset();
#if defined ANDROID || defined TVOS
	SDL_QuitSubSystem(JOYSTICK_SUBSYSTEMS);
#endif
}

void update()
{
	check_joysticks();

	// joystick repeat
	for (size_t j = 0; j < joysticks.size(); j++) {
		REPEAT_VEC &joystick_repeats = joysticks[j].joystick_repeats;
		for (size_t i = 0; i < joystick_repeats.size(); i++) {
			Joy_Repeat &jr = joystick_repeats[i];
			if (check_joystick_repeat(jr)) {
				jr.repeated = true;
				TGUI_Event event;
				if (jr.is_button) {
					event.type = TGUI_JOY_DOWN;
					event.joystick.is_repeat = true;
					event.joystick.button = jr.button;
					event.joystick.id = joysticks[j].id;
					shim::push_event(event);
				}
				else {
					event.type = TGUI_JOY_AXIS;
					event.joystick.is_repeat = true;
					event.joystick.axis = jr.axis;
					event.joystick.value = jr.value;
					event.joystick.id = joysticks[j].id;
					shim::push_event(event);
				}
			}
		}
	}

	// mouse button repeat
	if (shim::mouse_button_repeats) {
		for (size_t i = 0; i < mouse_button_repeats.size(); i++) {
			Mouse_Button_Repeat &mr = mouse_button_repeats[i];
			if (check_mouse_button_repeat(mr)) {
				TGUI_Event event;
				event.type = TGUI_MOUSE_DOWN;
				event.mouse.is_touch = mr.is_touch;
				event.mouse.is_repeat = true;
				event.mouse.button = mr.button;
				event.mouse.finger = mr.finger;
				event.mouse.x = mr.down_pos.x * shim::scale + shim::screen_offset.x;
				event.mouse.y = mr.down_pos.y * shim::scale + shim::screen_offset.y;
				event.mouse.normalised = false;
				shim::push_event(event);
			}
		}
	}
}

void handle_event(TGUI_Event *event)
{
	// FIXME: support multiple joysticks
	if (event->type == TGUI_JOY_DOWN || event->type == TGUI_JOY_UP || event->type == TGUI_JOY_AXIS) {
		Joystick *js = find_joystick(event->joystick.id);

		if (js == 0) {
			event->type = TGUI_UNKNOWN;
			return;
		}
	}

	// joystick button repeat
	if ((event->type == TGUI_JOY_DOWN || event->type == TGUI_JOY_UP)) {
		Joystick *js = find_joystick(event->joystick.id);

		if (js == 0) {
			return;
		}

		REPEAT_VEC &joystick_repeats = js->joystick_repeats;

		if (event->type == TGUI_JOY_DOWN) {
			if (find_joy_repeat(true, event->joystick.button, js) < 0 && event->joystick.is_repeat == false) {
				Joy_Repeat jr;
				jr.is_button = true;
				jr.button = event->joystick.button;
				jr.initial_press_time = SDL_GetTicks();
				jr.down = true;
				joystick_repeats.push_back(jr);
			}
		}
		else {
			int index = find_joy_repeat(true, event->joystick.button, js);
			if (index >= 0) {
				joystick_repeats.erase(joystick_repeats.begin() + index);
			}
		}
	}
	else if (event->type == TGUI_MOUSE_DOWN || event->type == TGUI_MOUSE_UP) {
		if (event->type == TGUI_MOUSE_DOWN) {
			if (find_mouse_button_repeat(event->mouse.is_touch, event->mouse.button, event->mouse.finger) < 0 && event->mouse.is_repeat == false) {
				Mouse_Button_Repeat mr;
				mr.is_touch = event->mouse.is_touch;
				mr.button = event->mouse.button;
				mr.finger = event->mouse.finger;
				mr.down_pos = {event->mouse.x, event->mouse.y};
				mr.initial_press_time = SDL_GetTicks();
				mr.down = true;
				mouse_button_repeats.push_back(mr);
			}
		}
		else {
			int index = find_mouse_button_repeat(event->mouse.is_touch, event->mouse.button, event->mouse.finger);
			if (index >= 0) {
				mouse_button_repeats.erase(mouse_button_repeats.begin() + index);
			}
		}
	}
	else if (event->type == TGUI_MOUSE_AXIS) {
		int index = find_mouse_button_repeat(event->mouse.is_touch, event->mouse.button, event->mouse.finger);
		if (index >= 0) {
			Mouse_Button_Repeat &mr = mouse_button_repeats[index];
			util::Point<float> diff = mr.down_pos - util::Point<float>(event->mouse.x, event->mouse.y);
			if (shim::mouse_button_repeat_max_movement >= 0 && (fabsf(diff.x) > shim::scale*shim::mouse_button_repeat_max_movement || fabsf(diff.y) > shim::scale*shim::mouse_button_repeat_max_movement)) {
				mouse_button_repeats.erase(mouse_button_repeats.begin() + index);
			}
			else {
				mr.down_pos = {event->mouse.x, event->mouse.y};
			}
		}
	}
}

bool convert_to_focus_event(TGUI_Event *event, Focus_Event *focus)
{
	int x = 0;
	int y = 0;

	if (is_joystick_connected() && event->type == TGUI_JOY_AXIS && (event->joystick.axis == 0 || event->joystick.axis == 1)) {
		Joystick *js = find_joystick(event->joystick.id);

		if (js == 0) {
			return false;
		}
		
		REPEAT_VEC &joystick_repeats = js->joystick_repeats;

		int axis = event->joystick.axis;
		int index = find_joy_repeat(false, axis, js);
#ifndef STEAM_INPUT // On Steam builds, we can't use JoystickGetAxis/etc because sometimes it's the dpad generating axis events
		Sint16 other_s = SDL_JoystickGetAxis(js->joy, 1-axis);
		float other = TGUI6_NORMALISE_JOY_AXIS(other_s);

		if (fabsf(event->joystick.value) > shim::joystick_activate_threshold && fabsf(other) < shim::joystick_deactivate_threshold) {
#else
		if (fabsf(event->joystick.value) > shim::joystick_activate_threshold) {
#endif
			bool go = true;
			if (index < 0) {
				if (event->joystick.is_repeat == false) {
					Joy_Repeat jr;
					jr.is_button = false;
					jr.axis = axis;
					jr.value = event->joystick.value < 0 ? -1 : 1;
					jr.initial_press_time = SDL_GetTicks();
					jr.down = true;
					jr.repeated = false;
					joystick_repeats.push_back(jr);
					go = true;
				}
			}
			else {
				Joy_Repeat &jr = joystick_repeats[index];
				if (jr.repeated) {
					jr.repeated = false;
					go = true;
				}
				else {
					go = false;
				}
			}
			if (go) {
				if (axis == 0) {
					if (event->joystick.value < 0) {
						x = -1;
					}
					else {
						x = 1;
					}
				}
				else if (axis == 1) {
					if (event->joystick.value < 0) {
						y = -1;
					}
					else {
						y = 1;
					}
				}
			}
		}
		else if (index >= 0 && fabsf(event->joystick.value) < shim::joystick_deactivate_threshold) {
			joystick_repeats.erase(joystick_repeats.begin() + index);
		}
	}

	if (event->type == TGUI_KEY_DOWN) {
		if (event->keyboard.code == shim::key_l) {
			x = -1;
		}
		else if (event->keyboard.code == shim::key_r) {
			x = 1;
		}
		else if (event->keyboard.code == shim::key_u) {
			y = -1;
		}
		else if (event->keyboard.code == shim::key_d) {
			y = 1;
		}
	}

	if (event->type == TGUI_JOY_DOWN) {
		if (event->joystick.button == TGUI_B_L) {
			x = -1;
		}
		else if (event->joystick.button == TGUI_B_R) {
			x = 1;
		}
		else if (event->joystick.button == TGUI_B_U) {
			y = -1;
		}
		else if (event->joystick.button == TGUI_B_D) {
			y = 1;
		}
	}

	if (x != 0 || y != 0) {
		focus->orig_type = event->type;
		if (event->type == TGUI_KEY_DOWN) {
			focus->u.orig_keyboard.code = event->keyboard.code;
			focus->u.orig_keyboard.is_repeat = event->keyboard.is_repeat;
		}
		else {
			focus->u.orig_joystick.id = event->joystick.id;
			focus->u.orig_joystick.button = event->joystick.button;
			focus->u.orig_joystick.axis = event->joystick.axis;
			focus->u.orig_joystick.value = event->joystick.value;
			focus->u.orig_joystick.is_repeat = event->joystick.is_repeat;
		}
		focus->type = TGUI_FOCUS;
		if  (x < 0) {
			focus->focus.type = TGUI_FOCUS_LEFT;
		}
		else if (x > 0) {
			focus->focus.type = TGUI_FOCUS_RIGHT;
		}
		else if (y < 0) {
			focus->focus.type = TGUI_FOCUS_UP;
		}
		else {
			focus->focus.type = TGUI_FOCUS_DOWN;
		}
		return true;
	}
	else {
		return false;
	}
}

void convert_focus_to_original(TGUI_Event *event)
{
	if (event->type == TGUI_FOCUS) {
		// grab the original...
		input::Focus_Event *focus = dynamic_cast<input::Focus_Event *>(event);
		if (focus) {
			event->type = focus->orig_type;
			if (focus->orig_type == TGUI_KEY_DOWN) {
				event->keyboard.code = focus->u.orig_keyboard.code;
				event->keyboard.is_repeat = focus->u.orig_keyboard.is_repeat;
			}
			else {
				event->joystick.id = focus->u.orig_joystick.id;
				event->joystick.button = focus->u.orig_joystick.button;
				event->joystick.axis = focus->u.orig_joystick.axis;
				event->joystick.value = focus->u.orig_joystick.value;
				event->joystick.is_repeat = focus->u.orig_joystick.is_repeat;
			}
		}
	}
}

#ifdef STEAM_INPUT
static int steam_rumble(void *data)
{
	Uint32 length = *((Uint32 *)&data);

	int freq = 50000;

	SteamController()->TriggerVibration(get_controller_handle(), freq, freq);

	SDL_Delay(length);

	SteamController()->TriggerVibration(get_controller_handle(), 0, 0);

	return 0;
}
#endif

// FIXME: this rumbles ALL joysticks
void rumble(Uint32 length)
{
#ifdef STEAM_INPUT
	if (shim::steam_init_failed == false) {
		SDL_Thread *thread = SDL_CreateThread(steam_rumble, "", (void *)(intptr_t)length);
		SDL_DetachThread(thread);
	}
	else
#endif
#ifdef ANDROID
	if (is_joystick_connected() == false) {
		JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
		jobject activity = (jobject)SDL_AndroidGetActivity();
		jclass clazz(env->GetObjectClass(activity));

		jmethodID method_id = env->GetMethodID(clazz, "rumble", "(I)V");

		if (method_id != 0) {
			env->CallVoidMethod(activity, method_id, length);
		}

		env->DeleteLocalRef(activity);
		env->DeleteLocalRef(clazz);
	}
	else
#elif defined IOS && !defined TVOS
	if (length >= 500) {
		AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
	}
	else
#endif
	{
		for (size_t i = 0; i < joysticks.size(); i++) {
			Joystick &j = joysticks[i];

			//SDL_GameControllerRumble(j.gc, strength*0xffff, strength*0xffff, length);
			if (j.gc) {
				SDL_GameControllerRumble(j.gc, 0x7777, 0x7777, length);
			}
		}
	}
}

bool is_joystick_connected()
{
	return joysticks.size() != 0;
}

#ifdef STEAM_INPUT
std::string get_joystick_button_name_steam(int button)
{
	switch (button) {
		case k_EControllerActionOrigin_A:
		case k_EControllerActionOrigin_XBoxOne_A:
		case k_EControllerActionOrigin_XBox360_A:
		case k_EControllerActionOrigin_SteamV2_A:
		case k_EControllerActionOrigin_Switch_A:
			return "@F0";
		case k_EControllerActionOrigin_B:
		case k_EControllerActionOrigin_XBoxOne_B:
		case k_EControllerActionOrigin_XBox360_B:
		case k_EControllerActionOrigin_SteamV2_B:
		case k_EControllerActionOrigin_Switch_B:
			return "@F1";
		case k_EControllerActionOrigin_X:
		case k_EControllerActionOrigin_XBox360_X:
		case k_EControllerActionOrigin_XBoxOne_X:
		case k_EControllerActionOrigin_SteamV2_X:
		case k_EControllerActionOrigin_Switch_X:
			return "@F2";
		case k_EControllerActionOrigin_Y:
		case k_EControllerActionOrigin_XBoxOne_Y:
		case k_EControllerActionOrigin_XBox360_Y:
		case k_EControllerActionOrigin_SteamV2_Y:
		case k_EControllerActionOrigin_Switch_Y:
			return "@F3";
		case k_EControllerActionOrigin_PS4_X:
			return "@F4";
		case k_EControllerActionOrigin_PS4_Circle:
			return "@F5";
		case k_EControllerActionOrigin_PS4_Triangle:
			return "@F7";
		case k_EControllerActionOrigin_PS4_Square:
			return "@F6";
		case k_EControllerActionOrigin_LeftBumper:
		case k_EControllerActionOrigin_PS4_LeftBumper:
		case k_EControllerActionOrigin_XBoxOne_LeftBumper:
		case k_EControllerActionOrigin_XBox360_LeftBumper:
		case k_EControllerActionOrigin_SteamV2_LeftBumper:
		case k_EControllerActionOrigin_Switch_LeftBumper:
			return "LB";
		case k_EControllerActionOrigin_RightBumper:
		case k_EControllerActionOrigin_PS4_RightBumper:
		case k_EControllerActionOrigin_XBoxOne_RightBumper:
		case k_EControllerActionOrigin_XBox360_RightBumper:
		case k_EControllerActionOrigin_SteamV2_RightBumper:
		case k_EControllerActionOrigin_Switch_RightBumper:
			return "RB";
		case k_EControllerActionOrigin_LeftStick_Click:
		case k_EControllerActionOrigin_PS4_LeftStick_Click:
		case k_EControllerActionOrigin_XBoxOne_LeftStick_Click:
		case k_EControllerActionOrigin_SteamV2_LeftStick_Click:
		case k_EControllerActionOrigin_XBox360_LeftStick_Click:
		case k_EControllerActionOrigin_Switch_LeftStick_Click:
			return "LS";
		case k_EControllerActionOrigin_PS4_RightStick_Click:
		case k_EControllerActionOrigin_XBoxOne_RightStick_Click:
		case k_EControllerActionOrigin_XBox360_RightStick_Click:
		case k_EControllerActionOrigin_Switch_RightStick_Click:
			return "RS";
		case k_EControllerActionOrigin_Start:
		case k_EControllerActionOrigin_XBox360_Start:
		case k_EControllerActionOrigin_SteamV2_Start:
			return "START";
		case k_EControllerActionOrigin_Back:
		case k_EControllerActionOrigin_XBox360_Back:
		case k_EControllerActionOrigin_SteamV2_Back:
			return "BACK";
		case k_EControllerActionOrigin_XBoxOne_Menu:
			return "MENU";
		case k_EControllerActionOrigin_XBoxOne_View:
			return "VIEW";
		case k_EControllerActionOrigin_PS4_Options:
			return "OPTIONS";
		case k_EControllerActionOrigin_PS4_Share:
			return "SHARE";
		case k_EControllerActionOrigin_Switch_Minus:
			return "-";
		case k_EControllerActionOrigin_Switch_Plus:
			return "+";
		case k_EControllerActionOrigin_Switch_Capture:
			return "CAPTURE";
	}

	const char *s = SteamController()->GetStringForActionOrigin((EControllerActionOrigin)button);

	return util::uppercase(s);
}

std::string get_joystick_button_colour_code_steam(int button)
{
	switch (button) {
		case k_EControllerActionOrigin_A:
		case k_EControllerActionOrigin_XBoxOne_A:
		case k_EControllerActionOrigin_XBox360_A:
		case k_EControllerActionOrigin_SteamV2_A:
			return "|40";
		case k_EControllerActionOrigin_B:
		case k_EControllerActionOrigin_XBoxOne_B:
		case k_EControllerActionOrigin_XBox360_B:
		case k_EControllerActionOrigin_SteamV2_B:
			return "|41";
		case k_EControllerActionOrigin_X:
		case k_EControllerActionOrigin_XBox360_X:
		case k_EControllerActionOrigin_XBoxOne_X:
		case k_EControllerActionOrigin_SteamV2_X:
			return "|42";
		case k_EControllerActionOrigin_Y:
		case k_EControllerActionOrigin_XBoxOne_Y:
		case k_EControllerActionOrigin_XBox360_Y:
		case k_EControllerActionOrigin_SteamV2_Y:
			return "|43";
		case k_EControllerActionOrigin_PS4_X:
			return "|47";
		case k_EControllerActionOrigin_PS4_Circle:
			return "|44";
		case k_EControllerActionOrigin_PS4_Triangle:
			return "|46";
		case k_EControllerActionOrigin_PS4_Square:
			return "|45";
		case k_EControllerActionOrigin_LeftBumper:
		case k_EControllerActionOrigin_PS4_LeftBumper:
		case k_EControllerActionOrigin_XBoxOne_LeftBumper:
		case k_EControllerActionOrigin_XBox360_LeftBumper:
		case k_EControllerActionOrigin_SteamV2_LeftBumper:
			return "";
		case k_EControllerActionOrigin_RightBumper:
		case k_EControllerActionOrigin_PS4_RightBumper:
		case k_EControllerActionOrigin_XBoxOne_RightBumper:
		case k_EControllerActionOrigin_XBox360_RightBumper:
		case k_EControllerActionOrigin_SteamV2_RightBumper:
			return "";
		case k_EControllerActionOrigin_LeftStick_Click:
		case k_EControllerActionOrigin_PS4_LeftStick_Click:
		case k_EControllerActionOrigin_XBoxOne_LeftStick_Click:
		case k_EControllerActionOrigin_SteamV2_LeftStick_Click:
		case k_EControllerActionOrigin_XBox360_LeftStick_Click:
			return "";
		case k_EControllerActionOrigin_PS4_RightStick_Click:
		case k_EControllerActionOrigin_XBoxOne_RightStick_Click:
		case k_EControllerActionOrigin_XBox360_RightStick_Click:
			return "";
		case k_EControllerActionOrigin_Start:
		case k_EControllerActionOrigin_XBox360_Start:
		case k_EControllerActionOrigin_SteamV2_Start:
			return "";
		case k_EControllerActionOrigin_Back:
		case k_EControllerActionOrigin_XBox360_Back:
		case k_EControllerActionOrigin_SteamV2_Back:
			return "";
		case k_EControllerActionOrigin_XBoxOne_Menu:
			return "";
		case k_EControllerActionOrigin_XBoxOne_View:
			return "";
		case k_EControllerActionOrigin_PS4_Options:
			return "";
		case k_EControllerActionOrigin_PS4_Share:
			return "";
	}

	return "";
}
#endif

std::string get_joystick_button_colour_code(int button)
{
	return "";

#ifdef STEAM_INPUT
	if (shim::steam_init_failed == false) {
		return get_joystick_button_colour_code_steam(button);
	}
#endif

	Joystick_Type type;

	if (joysticks.size() > 0) {
		type = joysticks[0].type;
	}
	else {
		type = XBOX;
	}

	if (type == PLAYSTATION) {
		switch (button) {
			case TGUI_B_A:
				return "|47";
			case TGUI_B_B:
				return "|44";
			case TGUI_B_X:
				return "|45";
			case TGUI_B_Y:
				return "|46";
			default:
				return "";
		}
	}
	else if (type == NINTENDO) {
		return "";
	}
	else {
#ifdef IOS
		if (type == IOS_JOY) {
			switch (button) {
				case TGUI_B_A:
					return "|41";
				case TGUI_B_B:
					return "|40";
				case TGUI_B_X:
					return "|43";
				case TGUI_B_Y:
					return "|42";
				default:
					return "";
			}
		}
		else
#endif
		{
			switch (button) {
				case TGUI_B_A:
					return "|40";
				case TGUI_B_B:
					return "|41";
				case TGUI_B_X:
					return "|42";
				case TGUI_B_Y:
					return "|43";
				default:
					return "";
			}
		}
	}

	return "";
}

std::string get_joystick_button_name(int button)
{
#ifdef STEAM_INPUT
	if (shim::steam_init_failed == false) {
		return get_joystick_button_name_steam(button);
	}
#endif

	Joystick_Type type;

	if (joysticks.size() > 0) {
		type = joysticks[0].type;
	}
	else {
		type = XBOX;
	}

	if (type == PLAYSTATION) {
		switch (button) {
			case TGUI_B_A:
				return "@F4";
			case TGUI_B_B:
				return "@F5";
			case TGUI_B_X:
				return "@F6";
			case TGUI_B_Y:
				return "@F7";
			case TGUI_B_LB:
				return "LB";
			case TGUI_B_RB:
				return "RB";
			case TGUI_B_BACK:
				return "SHARE";
			case TGUI_B_START:
				return "OPTIONS";
			case TGUI_B_GUIDE:
				return "PS";
			case TGUI_B_LS:
				return "LS";
			case TGUI_B_RS:
				return "RS";
			default:
				return std::string("BUTTON ") + util::itos(button);
		}
	}
	else if (type == NINTENDO) {
		switch (button) {
			case TGUI_B_A:
				return "A";
			case TGUI_B_B:
				return "B";
			case TGUI_B_X:
				return "X";
			case TGUI_B_Y:
				return "Y";
			case TGUI_B_LB:
				return "LB";
			case TGUI_B_RB:
				return "RB";
			case TGUI_B_BACK:
				return "-";
			case TGUI_B_START:
				return "+";
			case TGUI_B_GUIDE:
				return "HOME";
			case TGUI_B_LS:
				return "LS";
			case TGUI_B_RS:
				return "RS";
			default:
				return std::string("BUTTON ") + util::itos(button);
		}
	}
	else {
		switch (button) {
			case TGUI_B_A:
				return "@F0";
			case TGUI_B_B:
				return "@F1";
			case TGUI_B_X:
				return "@F2";
			case TGUI_B_Y:
				return "@F3";
			case TGUI_B_LB:
				return "LB";
			case TGUI_B_RB:
				return "RB";
			case TGUI_B_BACK:
				if (type == XBONE) {
					return "VIEW";
				}
				else {
					return "BACK";
				}
			case TGUI_B_START:
				if (type == XBONE) {
					return "MENU";
				}
				else {
					return "START";
				}
			case TGUI_B_GUIDE:
				if (type == XBONE) {
					return "XBOX";
				}
				else {
					return "GUIDE";
				}
			case TGUI_B_LS:
				return "LS";
			case TGUI_B_RS:
				return "RS";
			default:
				return std::string("BUTTON ") + util::itos(button);
		}
	}
}

void drop_repeats(bool joystick, bool mouse)
{
	if (joystick) {
		for (size_t i = 0; i < joysticks.size(); i++) {
			joysticks[i].joystick_repeats.clear();
		}
	}
	if (mouse) {
		mouse_button_repeats.clear();
	}
}

int get_num_joysticks()
{
	return joysticks.size();
}

SDL_JoystickID get_controller_id(int index)
{
	if (joysticks.size() <= index) {
		return 0;
	}
	return joysticks[index].id;
}

SDL_Joystick *get_sdl_joystick(SDL_JoystickID id)
{
	Joystick *j = find_joystick(id);
	if (j) {
		return j->joy;
	}
	else {
		return nullptr;
	}
}

#ifdef STEAM_INPUT
ControllerHandle_t get_controller_handle()
{
	if (joysticks.size() == 0) {
		return 0;
	}
	return joysticks[0].handle;
}
#endif

Focus_Event::~Focus_Event()
{
}

bool system_has_touchscreen()
{
	static bool cached = false;
	static bool result;

	if (cached) {
		return result;
	}

#ifdef _WIN32
	result = GetSystemMetrics(/*SM_MAXIMUMTOUCHES*/95) > 0; // SM_MAXIMUMTOUCHES is only available on Windows 7+
#elif defined ANDROID
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "has_touchscreen", "()Z");

	result = (bool)env->CallBooleanMethod(activity, method_id);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
#elif defined __linux__
	// hack for Chromebook where code below this if doesn't work
	if (shim::force_tablet) {
		return true;
	}

	// slurp_file_from_filesystem doesn't work here because these special files return 0 for size

	SDL_RWops *file = SDL_RWFromFile("/proc/bus/input/devices", "r");

	if (file == 0) {
		return false;
	}

	char *buf = new char[1024*100];
	int c = 0;

	while (SDL_RWread(file, buf+c, 1, 1) == 1 && c < 1024*100-2) {
		c++;
	}

	buf[c] = 0;

	SDL_RWclose(file);

	std::string s = buf;

	s = util::lowercase(s);
	if (s.find("touchscreen") != std::string::npos) {
		result = true;
	}
	else {
		result = false;
	}

	delete[] buf;
#elif defined TVOS
	result = false;
#elif defined IOS
	result = true;
#else	
	result = false;
#endif
	
	cached = true;

	return result;
}

bool system_has_keyboard()
{
#ifdef ANDROID
	return util::is_chromebook();
#elif defined IOS
	return false;
#elif defined _WIN32
	// Kind of a hack. If it's a tablet we want non-keyboard behaviour...
	if (shim::force_tablet) {
		return GetSystemMetrics(SM_TABLETPC) == 0;
	}
	else {
		return true;
	}
#elif defined __linux__
	if (shim::force_tablet) {
		return false;
	}
	else {
		return true;
	}
#else
	return true;
#endif
}


} // End namespace input

} // End namespace noo
