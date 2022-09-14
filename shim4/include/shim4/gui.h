#ifndef NOO_GUI_H
#define NOO_GUI_H

#include "shim4/main.h"
#include "shim4/shim.h"
#include "shim4/translation.h"

namespace noo {

namespace gui {

ALIGN(16) GUI {
public:
	TGUI *gui;
	TGUI_Widget *focus; // backup focus

	SHIM4_EXPORT GUI();
	SHIM4_EXPORT virtual ~GUI();

	SHIM4_EXPORT bool is_transitioning_in();
	SHIM4_EXPORT bool is_transitioning_out();
	SHIM4_EXPORT bool is_transition_out_finished();

	SHIM4_EXPORT virtual void handle_event(TGUI_Event *event);

	SHIM4_EXPORT virtual void update();
	SHIM4_EXPORT virtual void update_background(); // called when the GUI is not the foremost

	SHIM4_EXPORT void pre_draw(); // special stuff (starts transition timer)

	SHIM4_EXPORT virtual void draw_back();
	SHIM4_EXPORT virtual void draw();
	SHIM4_EXPORT virtual void draw_fore();

	SHIM4_EXPORT virtual void resize(util::Size<int> new_size);

	SHIM4_EXPORT virtual bool is_fullscreen(); // if the top gui returns true, other guis don't get drawn

	SHIM4_EXPORT bool do_return(bool ret);

	SHIM4_EXPORT virtual bool transition_done(bool transition_in); // return true to cancel and keep this GUI alive

	SHIM4_EXPORT virtual void transition_start(float p);
	SHIM4_EXPORT virtual void transition_end();

	// normally a fade is done if transitions are enabled, but these can be used instead
	SHIM4_EXPORT void use_enlarge_transition(bool onoff);
	SHIM4_EXPORT void use_shrink_transition(bool onoff);
	SHIM4_EXPORT void use_appear_transition(bool onoff);
	SHIM4_EXPORT void use_slide_transition(bool onoff);
	SHIM4_EXPORT void use_slide_vertical_transition(bool onoff);

	SHIM4_EXPORT void exit(); // call this to exit this GUI and remove it from shim::guis after transition and update()

	SHIM4_EXPORT virtual void lost_device();
	SHIM4_EXPORT virtual void found_device();

	SHIM4_EXPORT virtual void transition_in_done(); // called when transition in is done (only if transition is true)

	SHIM4_EXPORT void set_transition(bool transition);

	// For 16 byte alignment to make glm::mat4 able to use SIMD
#ifdef _WIN32
	SHIM4_EXPORT void *operator new(size_t i);
	SHIM4_EXPORT void operator delete(void* p);
#endif

protected:
	static const int MAX_FADE_SCALE = 10;
	
	static bool started_transition_timer;
	static Uint32 transition_start_time;

	SHIM4_EXPORT void fade_transition(float p);
	SHIM4_EXPORT void scale_transition(float scale);
	SHIM4_EXPORT void appear_in_transition(float p);
	SHIM4_EXPORT void appear_out_transition(float p);
	SHIM4_EXPORT void slide_transition(float x);
	SHIM4_EXPORT void slide_vertical_transition(float y);

	bool transition;
	bool transitioning_in;
	bool transitioning_out;
	bool transition_is_enlarge;
	bool transition_is_shrink;
	bool transition_is_appear;
	bool transition_is_slide;
	bool transition_is_slide_vertical;
	glm::mat4 mv_backup;
	glm::mat4 p_backup;
	float last_transition_p;
	float appear_save;
	gfx::Image *plasma;
	int transition_duration;
	float slide_save;
};

enum Popup_Type {
	OK = 0,
	YESNO = 1
};

// Functions
int SHIM4_EXPORT popup(std::string caption, std::string text, Popup_Type type);
int SHIM4_EXPORT fatalerror(std::string caption, std::string text, Popup_Type type, bool do_exit = false);

} // End namespace gui

} // End namespace noo

#endif // NOO_GUI_H
