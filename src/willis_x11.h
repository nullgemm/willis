#ifndef H_WILLIS_INTERNAL_X11
#define H_WILLIS_INTERNAL_X11

#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xkb.h>

struct willis_data_x11
{
	xcb_connection_t* x11_conn;
	xcb_window_t x11_root;
	xcb_window_t x11_window;
};

struct willis_x11
{
	xcb_connection_t* display_system;
	xcb_window_t x11_root;
	xcb_window_t x11_window;

	int32_t xkb_device_id;
	uint8_t xkb_event;

	xcb_xkb_select_events_details_t select_events_details;
};

#endif
