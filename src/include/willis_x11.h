#ifndef H_WILLIS_X11
#define H_WILLIS_X11

#include "willis.h"

#include <xcb/xcb.h>

struct willis_x11_data
{
	xcb_connection_t* conn;
	xcb_window_t window;
	xcb_window_t root;
};

#if !defined(WILLIS_SHARED)
void willis_prepare_init_x11(
	struct willis_config_backend* config);
#endif

#endif
