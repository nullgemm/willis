#ifndef H_WILLIS
#define H_WILLIS

#include "willis_events.h"

#include <stdbool.h>

// another useful union to keep a clean and uniform API
// otherwise `the willis_init` function signature would vary
union willis_display_system
{
	int descriptor;
	void* handle;
	void* app;
};

// willis context
struct willis
{
	// display system handle used for registering and
	// unregistering our input event handler
	union willis_display_system fd;

	// user callback executed by our event handler after
	// the translation to willis events is done
	//
	// we pass the willis context so it can use the
	// utf-8 keycode translation if needed
	void (*callback)(
		struct willis* willis,
		enum willis_event_code event_code,
		enum willis_event_state event_state,
		void* data);

	// user data for callback
	void* data;
};

// translate qwerty-mapped scancode-willis-equivalents to the right utf-8
// string using the target system's localization mechanism
void willis_keyboard_utf8(
	struct willis* willis,
	enum willis_event_code event_code,
	char* out);

// register our event handler and store the required user callback
void willis_init(
	struct willis* willis,
	union willis_display_system fd,
	void (*callback)(
		struct willis* willis,
		enum willis_event_code event_code,
		enum willis_event_state event_state,
		void* data),
	void* data);

// translate the target system's events to willis equivalents
void willis_handle_events(
	void* event,
	void* willis);

#endif
