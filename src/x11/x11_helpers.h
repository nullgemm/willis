#ifndef H_WILLIS_INTERNAL_X11_HELPERS
#define H_WILLIS_INTERNAL_X11_HELPERS

#include "include/willis.h"
#include "x11/x11.h"

void x11_helpers_select_events_cursor(
	struct willis* context,
	uint32_t mask,
	struct willis_error_info* error);

void x11_helpers_select_events_keyboard(
	struct willis* context,
	struct willis_error_info* error);

void x11_helpers_update_keymap(
	struct willis* context,
	struct willis_error_info* error);

enum willis_event_code x11_helpers_translate_button(
	xcb_button_t button);

void x11_helpers_handle_xkb(
	struct willis* context,
	xcb_generic_event_t* event,
	struct willis_error_info* error);

#endif
