#ifndef H_WILLIS_INTERNAL_WIN_HELPERS
#define H_WILLIS_INTERNAL_WIN_HELPERS

#include "include/willis.h"
#include "win/win.h"

void win_helpers_select_events_cursor(
	struct willis* context,
	uint32_t mask,
	struct willis_error_info* error);

void win_helpers_select_events_keyboard(
	struct willis* context,
	struct willis_error_info* error);

void win_helpers_update_keymap(
	struct willis* context,
	struct willis_error_info* error);

enum willis_event_code win_helpers_translate_button(
	xcb_button_t button);

void win_helpers_handle_xkb(
	struct willis* context,
	xcb_generic_event_t* event,
	struct willis_error_info* error);

#endif
