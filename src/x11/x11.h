#ifndef H_WILLIS_X11_PRIVATE
#define H_WILLIS_X11_PRIVATE

#include "willis.h"
#include "common/willis_error.h"

#include <stdbool.h>
#include <xcb/xcb.h>

struct x11_backend
{
	xcb_connection_t* conn;
	xcb_window_t window;
	xcb_window_t root;
	uint8_t event;
};

void willis_x11_init(
	struct willis* context,
	struct willis_error_info* error);

void willis_x11_start(
	struct willis* context,
	void* data,
	struct willis_error_info* error);

bool willis_x11_handle_event(
	struct willis* context,
	void* event,
	struct willis_event_info* event_info,
	struct willis_error_info* error);

void willis_x11_mouse_grab(
	struct willis* context,
	struct willis_error_info* error);

void willis_x11_mouse_ungrab(
	struct willis* context,
	struct willis_error_info* error);

void willis_x11_stop(
	struct willis* context,
	struct willis_error_info* error);

void willis_x11_clean(
	struct willis* context,
	struct willis_error_info* error);

#endif
