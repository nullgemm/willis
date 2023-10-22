#include "include/willis.h"
#include "common/willis_private.h"
#include "include/willis_wayland.h"
#include "nix/nix.h"
#include "wayland/wayland.h"
#include "wayland/wayland_helpers.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include "zwp-relative-pointer-protocol.h"
#include "zwp-pointer-constraints-protocol.h"

void willis_wayland_init(
	struct willis* context,
	struct willis_error_info* error)
{
	// init Wayland struct
	struct wayland_backend* backend = malloc(sizeof (struct wayland_backend));

	if (backend == NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_ALLOC);
		return;
	}

	struct wayland_backend zero = {0};
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

	// mouse pointer listener
	struct wl_pointer_listener listener_pointer =
	{
		.enter = wayland_helpers_listener_pointer_enter,
		.leave = wayland_helpers_listener_pointer_leave,
		.motion = wayland_helpers_listener_pointer_motion,
		.button = wayland_helpers_listener_pointer_button,
		.axis = wayland_helpers_listener_pointer_axis,
		.frame = wayland_helpers_listener_pointer_frame,
		.axis_source = wayland_helpers_listener_pointer_axis_source,
		.axis_stop = wayland_helpers_listener_pointer_axis_stop,
		.axis_discrete = wayland_helpers_listener_pointer_axis_discrete,
	};

	backend->listener_pointer = listener_pointer;

	// keyboard listener
	struct wl_keyboard_listener listener_keyboard =
	{
		.keymap = wayland_helpers_listener_keyboard_keymap,
		.enter = wayland_helpers_listener_keyboard_enter,
		.leave = wayland_helpers_listener_keyboard_leave,
		.key = wayland_helpers_listener_keyboard_key,
		.modifiers = wayland_helpers_listener_keyboard_modifiers,
		.repeat_info = wayland_helpers_listener_keyboard_repeat_info,
	};

	backend->listener_keyboard = listener_keyboard;

	// relative mouse pointer listener
	struct zwp_relative_pointer_v1_listener listener_pointer_relative =
	{
		.relative_motion = wayland_helpers_listener_pointer_relative,
	};

	backend->listener_pointer_relative = listener_pointer_relative;

	// locked mouse pointer listener
	struct zwp_locked_pointer_v1_listener listener_pointer_locked =
	{
		.locked = wayland_helpers_listener_pointer_locked,
		.unlocked = wayland_helpers_listener_pointer_unlocked,
	};

	backend->listener_pointer_locked = listener_pointer_locked;

	// success
	willis_error_ok(error);
}

void willis_wayland_start(
	struct willis* context,
	void* data,
	struct willis_error_info* error)
{
	struct wayland_backend* backend = context->backend_data;
	struct willis_wayland_data* window_data = data;

	backend->mouse_grabbed = false;
	backend->xkb_device_id = 0;
	backend->xkb_event = 0;

	// get the best locale setting available
	willis_xkb_init_locale(backend->xkb_common);

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

	// add our registry handler callback
	window_data->add_registry_handler(
		window_data->add_registry_handler_data,
		willis_wayland_helpers_registry_handler,
		context);

	// add our capabilities handler callback
	window_data->add_capabilities_handler(
		window_data->add_capabilities_handler_data,
		willis_wayland_helpers_capabilities_handler,
		context);

	willis_error_ok(error);
}

void willis_wayland_handle_event(
	struct willis* context,
	void* event,
	struct willis_event_info* event_info,
	struct willis_error_info* error)
{
	struct wayland_backend* backend = context->backend_data;
	struct willis_xkb* xkb_common = backend->xkb_common;
	uint32_t* serial = event;

	if (serial == NULL)
	{
		willis_error_ok(error);
		return;
	}

	if (*serial == backend->event_serial)
	{
		*event_info = backend->event_info;
	}

	willis_error_ok(error);

#if 0
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

			event_code = wayland_helpers_translate_button(button_press->detail);
			event_state = WILLIS_STATE_PRESS;

			switch (event_code)
			{
				case WILLIS_MOUSE_WHEEL_UP:
				case WILLIS_MOUSE_WHEEL_DOWN:
				{
					event_info->mouse_wheel_steps = 1;
					break;
				}
				default:
				{
					break;
				}
			}

			break;
		}
		case XCB_BUTTON_RELEASE:
		{
			xcb_button_release_event_t* button_release =
				(xcb_button_release_event_t*) event;

			event_code = wayland_helpers_translate_button(button_release->detail);
			event_state = WILLIS_STATE_RELEASE;

			switch (event_code)
			{
				case WILLIS_MOUSE_WHEEL_UP:
				case WILLIS_MOUSE_WHEEL_DOWN:
				{
					event_info->mouse_wheel_steps = 1;
					break;
				}
				default:
				{
					break;
				}
			}

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
				wayland_helpers_handle_xkb(
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
#endif
}

bool willis_wayland_mouse_grab(
	struct willis* context,
	struct willis_error_info* error)
{
	struct wayland_backend* backend = context->backend_data;
	int error_posix;

	// abort if already grabbed
	if (backend->mouse_grabbed == true)
	{
		willis_error_ok(error);
		return false;
	}

	// check we have everything we need
	if (backend->pointer == NULL)
	{
		willis_error_throw(
			context,
			error,
			WILLIS_ERROR_WAYLAND_POINTER_MISSING);

		return false;
	}

	if (backend->pointer_relative_manager == NULL)
	{
		willis_error_throw(
			context,
			error,
			WILLIS_ERROR_WAYLAND_POINTER_RELATIVE_MANAGER_MISSING);

		return false;
	}

	if (backend->pointer_constraints_manager == NULL)
	{
		willis_error_throw(
			context,
			error,
			WILLIS_ERROR_WAYLAND_POINTER_CONSTRAINTS_MANAGER_MISSING);

		return false;
	}

	if (backend->pointer_surface == NULL)
	{
		willis_error_throw(
			context,
			error,
			WILLIS_ERROR_WAYLAND_POINTER_SURFACE_MISSING);

		return false;
	}

	// hide cursor
	wl_pointer_set_cursor(
		backend->pointer,
		0,
		NULL,
		0,
		0);

	// register relative mouse events listener
	backend->pointer_relative =
		zwp_relative_pointer_manager_v1_get_relative_pointer(
			backend->pointer_relative_manager,
			backend->pointer);

	if (backend->pointer_relative == NULL)
	{
		willis_error_throw(
			context,
			error,
			WILLIS_ERROR_WAYLAND_POINTER_RELATIVE_GET);

		return false;
	}

	error_posix =
		zwp_relative_pointer_v1_add_listener(
			backend->pointer_relative,
			&backend->listener_pointer_relative,
			context);

	if (error_posix == -1)
	{
		willis_error_throw(
			context,
			error,
			WILLIS_ERROR_WAYLAND_LISTENER_ADD);

		return false;
	}

	// grab pointer
	backend->pointer_locked =
		zwp_pointer_constraints_v1_lock_pointer(
			backend->pointer_constraints_manager,
			backend->pointer_surface,
			backend->pointer,
			NULL,
			ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);

	if (backend->pointer_locked == NULL)
	{
		willis_error_throw(
			context,
			error,
			WILLIS_ERROR_WAYLAND_POINTER_LOCKED_GET);

		return false;
	}

	error_posix =
		zwp_locked_pointer_v1_add_listener(
			backend->pointer_locked,
			&backend->listener_pointer_locked,
			context);

	if (error_posix == -1)
	{
		willis_error_throw(
			context,
			error,
			WILLIS_ERROR_WAYLAND_LISTENER_ADD);

		return false;
	}

	// all good
	backend->mouse_grabbed = true;
	willis_error_ok(error);
	return true;
}

bool willis_wayland_mouse_ungrab(
	struct willis* context,
	struct willis_error_info* error)
{
	struct wayland_backend* backend = context->backend_data;

	// abort if already ungrabbed
	if (backend->mouse_grabbed == false)
	{
		willis_error_ok(error);
		return false;
	}

	// check we have everything we need
	if (backend->pointer_relative == NULL)
	{
		willis_error_throw(
			context,
			error,
			WILLIS_ERROR_WAYLAND_POINTER_RELATIVE_MISSING);

		return false;
	}

	if (backend->pointer_locked == NULL)
	{
		willis_error_throw(
			context,
			error,
			WILLIS_ERROR_WAYLAND_POINTER_LOCKED_MISSING);

		return false;
	}

	// restore classic mouse pointer behaviour
	zwp_relative_pointer_v1_destroy(backend->pointer_relative);
	zwp_locked_pointer_v1_destroy(backend->pointer_locked);
	backend->pointer_relative = NULL;
	backend->pointer_locked = NULL;

	// all good
	backend->mouse_grabbed = false;
	willis_error_ok(error);
	return true;
}

void willis_wayland_stop(
	struct willis* context,
	struct willis_error_info* error)
{
	struct wayland_backend* backend = context->backend_data;
	struct willis_xkb* xkb_common = backend->xkb_common;

	xkb_state_unref(xkb_common->state);
	xkb_keymap_unref(xkb_common->keymap);
	xkb_compose_table_unref(xkb_common->compose_table);
	xkb_compose_state_unref(xkb_common->compose_state);
	xkb_context_unref(xkb_common->context);

	willis_error_ok(error);
}

void willis_wayland_clean(
	struct willis* context,
	struct willis_error_info* error)
{
	struct wayland_backend* backend = context->backend_data;
	struct willis_xkb* xkb_common = backend->xkb_common;

	free(xkb_common);
	free(backend);

	willis_error_ok(error);
}

void willis_prepare_init_wayland(
	struct willis_config_backend* config)
{
	config->data = NULL;
	config->init = willis_wayland_init;
	config->start = willis_wayland_start;
	config->handle_event = willis_wayland_handle_event;
	config->mouse_grab = willis_wayland_mouse_grab;
	config->mouse_ungrab = willis_wayland_mouse_ungrab;
	config->stop = willis_wayland_stop;
	config->clean = willis_wayland_clean;
}
