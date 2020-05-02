#ifndef H_WILLIS
#define H_WILLIS

#include "willis_events.h"

#ifdef WILLIS_X11
	#include <xcb/xcb.h>
	#include <xcb/xkb.h>
	#include <xkbcommon/xkbcommon-x11.h>
#endif

#include <stdbool.h>
#include <stdlib.h>

// willis context
struct willis
{
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
	size_t utf8_size;
	char* utf8_string;
	bool get_utf8;
	
	// cursor info for the user to copy
	int16_t mouse_x;
	int16_t mouse_y;

	// internal x11-specific structures
#ifdef WILLIS_X11
	xcb_connection_t* display_system;

	struct xkb_context* xkb_ctx;
	int32_t xkb_device_id;
	uint8_t xkb_event;

	struct xkb_keymap* xkb_keymap;
    struct xkb_state* xkb_state;

	xcb_xkb_select_events_details_t select_events_details;
#endif
};

// register our event handler and store the required user callback
bool willis_init(
	struct willis* willis,
	void* display_system,
	bool utf8,
	void (*callback)(
		struct willis* willis,
		enum willis_event_code event_code,
		enum willis_event_state event_state,
		void* data),
	void* data);

// translate the target system's events to willis equivalents
void willis_handle_events(
	void* event,
	void* ctx);

#endif
