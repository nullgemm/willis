#ifndef H_WILLIS
#define H_WILLIS

#include "willis_events.h"

#if defined(WILLIS_X11)
	#include "willis_x11.h"
	#include "willis_xkb.h"
#elif defined(WILLIS_WAYLAND)
	#include "willis_wayland.h"
	#include "willis_xkb.h"
#elif defined(WILLIS_MACOS)
	#include "willis_macos.h"
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// willis context
struct willis
{
	// platform-specific context
	#if defined(WILLIS_X11)
	struct willis_x11 willis_x11;
	struct willis_xkb willis_xkb;
	#elif defined(WILLIS_WAYLAND)
	struct willis_wayland willis_wayland;
	struct willis_xkb willis_xkb;
	#elif defined(WILLIS_MACOS)
	struct willis_macos willis_macos;
	#endif

	// user callback executed by our event handler after
	// the translation to willis events is done
	void (*callback)(
		struct willis* willis,
		enum willis_event_code event_code,
		enum willis_event_state event_state,
		void* data);

	// user data for callback
	void* data;

	// utf-8 input string for the user to copy
	bool get_utf8;
	char* utf8_string;
	size_t utf8_size;
	
	// cursor info for the user to copy
	bool mouse_grab;
	int16_t mouse_x;
	int16_t mouse_y;

	// signed fixed-point (Q31.32)
	int64_t diff_x;
	int64_t diff_y;
};

// register our event handler and store the required user callback
bool willis_init(
	struct willis* willis,
	void* backend_link,
	bool utf8,
	void (*callback)(
		struct willis* willis,
		enum willis_event_code event_code,
		enum willis_event_state event_state,
		void* data),
	void* data);

// translate the target system's events to willis equivalents
void willis_handle_events(void* event, void* ctx);

// lock mouse to application and get movement deltas
bool willis_mouse_grab(struct willis* willis);
bool willis_mouse_ungrab(struct willis* willis);

// close willis and free all resources
bool willis_free(struct willis* willis);

// getters
char* willis_get_utf8_string(struct willis* willis);
bool willis_get_mouse_grab(struct willis* willis);
int16_t willis_get_mouse_x(struct willis* willis);
int16_t willis_get_mouse_y(struct willis* willis);
int64_t willis_get_diff_x(struct willis* willis);
int64_t willis_get_diff_y(struct willis* willis);

#endif
