#include "include/willis.h"
#include "common/willis_private.h"
#include "include/willis_x11.h"
#include "nix/nix.h"
#include "x11/x11.h"
#include "x11/x11_helpers.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xfixes.h>
#include <xcb/xinput.h>
#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon-x11.h>

void willis_x11_init(
	struct willis* context,
	struct willis_error_info* error)
{
	// init x11 struct
	struct x11_backend* backend = malloc(sizeof (struct x11_backend));

	if (backend == NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_ALLOC);
		return;
	}

	struct x11_backend zero = {0};
	*backend = zero;

	context->backend_data = backend;

	// init xkb struct
	struct willis_xkb* xkb_common = malloc(sizeof (struct willis_xkb));

	if (xkb_common == NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_ALLOC);
		return;
	}

	struct willis_xkb zero_xkb = {0};
	*xkb_common = zero_xkb;

	backend->xkb_common = xkb_common;

	// success
	willis_error_ok(error);
}

void willis_x11_start(
	struct willis* context,
	void* data,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;
	struct willis_x11_data* window_data = data;

	xcb_xkb_select_events_details_t zero = {0};
	xcb_generic_error_t* error_xcb;

	backend->conn = window_data->conn;
	backend->window = window_data->window;
	backend->root = window_data->root;
	backend->mouse_grabbed = false;

	backend->xkb_device_id = 0;
	backend->xkb_event = 0;
	backend->xkb_select_events_details = zero;

	// get the best locale setting available
	willis_xkb_init_locale(backend->xkb_common);

	// init the xkb x11 extension
	int error_posix =
		xkb_x11_setup_xkb_extension(
			backend->conn,
			XKB_X11_MIN_MAJOR_XKB_VERSION,
			XKB_X11_MIN_MINOR_XKB_VERSION,
			XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
			NULL,
			NULL,
			&(backend->xkb_event),
			NULL);

	if (error_posix == 0)
	{
		willis_error_throw(context, error, WILLIS_ERROR_X11_XKB_SETUP);
		return;
	}

	// create the xkb context
	backend->xkb_common->context =
		xkb_context_new(
			XKB_CONTEXT_NO_FLAGS);

	if (backend->xkb_common->context == NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_XKB_CONTEXT_NEW);
		return;
	}

	// prepare composition handling with xkb
	willis_xkb_init_compose(backend->xkb_common);

	// get the xkb device id
	backend->xkb_device_id =
		xkb_x11_get_core_keyboard_device_id(
			backend->conn);

	if (backend->xkb_device_id == -1)
	{
		xkb_compose_state_unref(backend->xkb_common->compose_state); // ok to unref if NULL
		xkb_compose_table_unref(backend->xkb_common->compose_table); // ok to unref if NULL
		xkb_context_unref(backend->xkb_common->context);
		willis_error_throw(context, error, WILLIS_ERROR_X11_XKB_DEVICE_GET);
		return;
	}

	// update the xkb keymap
	x11_helpers_update_keymap(context, error);

	if (willis_error_get_code(error) != WILLIS_ERROR_OK)
	{
		xkb_compose_state_unref(backend->xkb_common->compose_state);
		xkb_compose_table_unref(backend->xkb_common->compose_table);
		xkb_context_unref(backend->xkb_common->context);
		return;
	}

	// select xkb events
	x11_helpers_select_events_keyboard(context, error);

	if (willis_error_get_code(error) != WILLIS_ERROR_OK)
	{
		xkb_state_unref(backend->xkb_common->state);
		xkb_keymap_unref(backend->xkb_common->keymap);
		xkb_compose_state_unref(backend->xkb_common->compose_state);
		xkb_compose_table_unref(backend->xkb_common->compose_table);
		xkb_context_unref(backend->xkb_common->context);
		return;
	}

	willis_error_ok(error);
}

void willis_x11_handle_event(
	struct willis* context,
	void* event,
	struct willis_event_info* event_info,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;
	struct willis_xkb* xkb_common = backend->xkb_common;

	// initialize event code and state to default values
	enum willis_event_code event_code = WILLIS_NONE;
	enum willis_event_state event_state = WILLIS_STATE_NONE;

	// initialize here to make the switch below more readable
	willis_error_ok(error);
	event_info->event_code = event_code;
	event_info->event_state = event_state;
	event_info->utf8_string = NULL;
	event_info->utf8_size = 0;
	event_info->mouse_x = 0;
	event_info->mouse_y = 0;
	event_info->diff_x = 0;
	event_info->diff_y = 0;

	// handle event
	xcb_generic_event_t* xcb_event = event;
	int code = xcb_event->response_type & ~0x80;

	switch (code)
	{
		case XCB_KEY_PRESS:
		{
			xcb_key_press_event_t* key_press =
				(xcb_key_press_event_t*) event;

			event_code = willis_xkb_translate_keycode(key_press->detail);
			event_state = WILLIS_STATE_PRESS;

			// use compose functions if available
			if (xkb_common->compose_state != NULL)
			{
				willis_xkb_utf8_compose(
					context,
					xkb_common,
					(xkb_keycode_t) key_press->detail,
					&(event_info->utf8_string),
					&(event_info->utf8_size),
					error);
			}
			// use simple keycode translation otherwise
			else
			{
				willis_xkb_utf8_simple(
					context,
					xkb_common,
					(xkb_keycode_t) key_press->detail,
					&(event_info->utf8_string),
					&(event_info->utf8_size),
					error);
			}

			break;
		}
		case XCB_KEY_RELEASE:
		{
			xcb_key_release_event_t* key_release =
				(xcb_key_release_event_t*) event;

			event_code = willis_xkb_translate_keycode(key_release->detail);
			event_state = WILLIS_STATE_RELEASE;

			break;
		}
		case XCB_BUTTON_PRESS:
		{
			xcb_button_press_event_t* button_press =
				(xcb_button_press_event_t*) event;

			event_code = x11_helpers_translate_button(button_press->detail);
			event_state = WILLIS_STATE_PRESS;

			break;
		}
		case XCB_BUTTON_RELEASE:
		{
			xcb_button_release_event_t* button_release =
				(xcb_button_release_event_t*) event;

			event_code = x11_helpers_translate_button(button_release->detail);
			event_state = WILLIS_STATE_RELEASE;

			break;
		}
		case XCB_MOTION_NOTIFY:
		{
			xcb_motion_notify_event_t* motion =
				(xcb_motion_notify_event_t*) event;

			event_code = WILLIS_MOUSE_MOTION;
			event_state = WILLIS_STATE_NONE;

			event_info->mouse_x = motion->event_x;
			event_info->mouse_y = motion->event_y;

			break;
		}
		case XCB_GE_GENERIC:
		{
			xcb_ge_generic_event_t* generic =
				(xcb_ge_generic_event_t*) event;

			if (generic->event_type != XCB_INPUT_RAW_MOTION)
			{
				break;
			}

			xcb_input_raw_motion_event_t* raw
				= (xcb_input_raw_motion_event_t*) event;

			int len =
				xcb_input_raw_button_press_axisvalues_length(raw);

			xcb_input_fp3232_t* axis =
				xcb_input_raw_button_press_axisvalues_raw(raw);

			xcb_input_fp3232_t value;

			if (len > 0)
			{
				value = axis[0];
				event_info->diff_x = (((int64_t) value.integral) << 32) | value.frac;
			}

			if (len > 1)
			{
				value = axis[1];
				event_info->diff_y = (((int64_t) value.integral) << 32) | value.frac;
			}

			event_code = WILLIS_MOUSE_MOTION;
			event_state = WILLIS_STATE_NONE;

			break;
		}
		default:
		{
			if (code == backend->xkb_event)
			{
				x11_helpers_handle_xkb(
					context,
					event,
					error);
			}

			break;
		}
	}

	event_info->event_code = event_code;
	event_info->event_state = event_state;

	// error always set
}

bool willis_x11_mouse_grab(
	struct willis* context,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;

	// abort if already grabbed
	if (backend->mouse_grabbed == true)
	{
		willis_error_ok(error);
		return false;
	}

	// check xfixes supports what we are about to attempt
	xcb_generic_error_t* error_xcb = NULL;

	xcb_xfixes_query_version_cookie_t xfixes_version_cookie =
		xcb_xfixes_query_version(
			backend->conn,
			4,
			0);
	
	xcb_xfixes_query_version_reply_t* xfixes_version_reply =
		xcb_xfixes_query_version_reply(
			backend->conn,
			xfixes_version_cookie,
			&error_xcb);

	if (error_xcb != NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_X11_XFIXES_VERSION);
		return false;
	}

	free(xfixes_version_reply);

	// hide the cursor just in case
	xcb_void_cookie_t xfixes_hide_cookie =
		xcb_xfixes_hide_cursor(
			backend->conn,
			backend->window);

	error_xcb =
		xcb_request_check(
			backend->conn,
			xfixes_hide_cookie);

	if (error_xcb != NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_X11_XFIXES_HIDE);
		return false;
	}

	// grab the pointer
	xcb_grab_pointer_cookie_t pointer_cookie =
		xcb_grab_pointer(
			backend->conn,
			true,
			backend->root,
			XCB_NONE,
			XCB_GRAB_MODE_ASYNC,
			XCB_GRAB_MODE_ASYNC,
			backend->window,
			XCB_CURSOR_NONE, // TODO use invisible cursor
			XCB_CURRENT_TIME);

	xcb_grab_pointer_reply_t* pointer_reply =
		xcb_grab_pointer_reply(
			backend->conn,
			pointer_cookie,
			&error_xcb);

	if (error_xcb != NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_X11_GRAB);
		return false;
	}

	// select events
	x11_helpers_select_events_cursor(
		context,
		XCB_INPUT_XI_EVENT_MASK_RAW_MOTION,
		error);

	if (willis_error_get_code(error) != WILLIS_ERROR_OK)
	{
		return false;
	}

	// error always set
	return true;
}

bool willis_x11_mouse_ungrab(
	struct willis* context,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;
	xcb_generic_error_t* error_xcb = NULL;
	xcb_void_cookie_t cookie;

	// abort if already ungrabbed
	if (backend->mouse_grabbed == false)
	{
		willis_error_ok(error);
		return false;
	}

	// ungrab the pointer
	cookie =
		xcb_ungrab_pointer(
			backend->conn,
			XCB_CURRENT_TIME);

	error_xcb =
		xcb_request_check(
			backend->conn,
			cookie);

	if (error_xcb != NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_X11_UNGRAB);
		return false;
	}

	// show the cursor back
	cookie =
		xcb_xfixes_show_cursor(
			backend->conn,
			backend->window);

	error_xcb =
		xcb_request_check(
			backend->conn,
			cookie);

	if (error_xcb != NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_X11_XFIXES_SHOW);
		return false;
	}

	// select events
	x11_helpers_select_events_cursor(
		context,
		0,
		error);

	if (willis_error_get_code(error) != WILLIS_ERROR_OK)
	{
		return false;
	}

	// error always set
	return true;
}

void willis_x11_stop(
	struct willis* context,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;
	struct willis_xkb* xkb_common = backend->xkb_common;

	xkb_state_unref(xkb_common->state);
	xkb_keymap_unref(xkb_common->keymap);
	xkb_compose_table_unref(xkb_common->compose_table);
	xkb_compose_state_unref(xkb_common->compose_state);
	xkb_context_unref(xkb_common->context);

	willis_error_ok(error);
}

void willis_x11_clean(
	struct willis* context,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;
	struct willis_xkb* xkb_common = backend->xkb_common;

	free(xkb_common);
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
