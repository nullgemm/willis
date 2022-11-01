#include "include/willis.h"
#include "common/willis_private.h"
#include "include/willis_x11.h"
#include "x11/x11.h"
#include "nix/nix.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_xrm.h>

void willis_x11_init(
	struct willis* context,
	struct willis_error_info* error)
{
	struct x11_backend* backend = malloc(sizeof (struct x11_backend));

	if (backend == NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_ALLOC);
		return;
	}

	struct x11_backend zero = {0};
	*backend = zero;

	context->backend_data = backend;

	willis_error_ok(error);
}

void willis_x11_start(
	struct willis* context,
	void* data,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;
	struct willis_x11_data* window_data = data;
	xcb_generic_error_t* error_xcb;

	backend->conn = window_data->conn;
	backend->window = window_data->window;
	backend->root = window_data->root;

	// TODO fill

	willis_error_ok(error);
}

bool willis_x11_handle_event(
	struct willis* context,
	void* event,
	struct willis_event_info* event_info,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;

	// hande configure notify and screen update events
	xcb_generic_event_t* xcb_event = event;

	// only lock the main mutex when making changes to the context
	int code = xcb_event->response_type & ~0x80;

	// TODO adapt
#if 0
	if (code == XCB_CONFIGURE_NOTIFY)
	{
		xcb_configure_notify_event_t* configure =
			(xcb_configure_notify_event_t*) xcb_event;

		// translate position in screen coordinates
		xcb_generic_error_t* error_xcb;

		xcb_translate_coordinates_cookie_t cookie_translate =
			xcb_translate_coordinates(
				backend->conn,
				backend->window,
				backend->root,
				0,
				0);

		xcb_translate_coordinates_reply_t* reply_translate =
			xcb_translate_coordinates_reply(
				backend->conn,
				cookie_translate,
				&error_xcb);

		if (error_xcb != NULL)
		{
			willis_error_throw(context, error, WILLIS_ERROR_X11_TRANSLATE);
			return false;
		}

		backend->window_x = reply_translate->dst_x;
		backend->window_y = reply_translate->dst_y;
		backend->window_width = configure->width;
		backend->window_height = configure->height;

		free(reply_translate);
	}
	else if (code == backend->event)
	{
		willis_refresh_display_list(context, error);

		if (willis_error_get_code(error) != WILLIS_ERROR_OK)
		{
			return false;
		}
	}
	else
	{
		return false;
	}
#endif

	bool valid = false;

	// TODO process

	return valid;
}

void willis_x11_mouse_grab(
	struct willis* context,
	struct willis_error_info* error)
{
}

void willis_x11_mouse_ungrab(
	struct willis* context,
	struct willis_error_info* error)
{
}

void willis_x11_stop(
	struct willis* context,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;

	willis_error_ok(error);
}

void willis_x11_clean(
	struct willis* context,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;

	if (context->event_info != NULL)
	{
		free(context->event_info);
	}

	free(backend);

	willis_error_ok(error);
}

void willis_prepare_init_x11(
	struct willis_config_backend* config)
{
	config->data = NULL;
	config->init = willis_x11_init;
	config->start = willis_x11_start;
	config->handle_event = willis_x11_handle_event;
	config->mouse_grab = willis_x11_mouse_grab;
	config->mouse_ungrab = willis_x11_mouse_ungrab;
	config->stop = willis_x11_stop;
	config->clean = willis_x11_clean;
}
