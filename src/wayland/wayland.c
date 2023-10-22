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
		.enter = willis_wayland_helpers_listener_pointer_enter,
		.leave = willis_wayland_helpers_listener_pointer_leave,
		.motion = willis_wayland_helpers_listener_pointer_motion,
		.button = willis_wayland_helpers_listener_pointer_button,
		.axis = willis_wayland_helpers_listener_pointer_axis,
		.frame = willis_wayland_helpers_listener_pointer_frame,
		.axis_source = willis_wayland_helpers_listener_pointer_axis_source,
		.axis_stop = willis_wayland_helpers_listener_pointer_axis_stop,
		.axis_discrete = willis_wayland_helpers_listener_pointer_axis_discrete,
	};

	backend->listener_pointer = listener_pointer;

	// keyboard listener
	struct wl_keyboard_listener listener_keyboard =
	{
		.keymap = willis_wayland_helpers_listener_keyboard_keymap,
		.enter = willis_wayland_helpers_listener_keyboard_enter,
		.leave = willis_wayland_helpers_listener_keyboard_leave,
		.key = willis_wayland_helpers_listener_keyboard_key,
		.modifiers = willis_wayland_helpers_listener_keyboard_modifiers,
		.repeat_info = willis_wayland_helpers_listener_keyboard_repeat_info,
	};

	backend->listener_keyboard = listener_keyboard;

	// relative mouse pointer listener
	struct zwp_relative_pointer_v1_listener listener_pointer_relative =
	{
		.relative_motion = willis_wayland_helpers_listener_pointer_relative,
	};

	backend->listener_pointer_relative = listener_pointer_relative;

	// locked mouse pointer listener
	struct zwp_locked_pointer_v1_listener listener_pointer_locked =
	{
		.locked = willis_wayland_helpers_listener_pointer_locked,
		.unlocked = willis_wayland_helpers_listener_pointer_unlocked,
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
	backend->event_callback = window_data->event_callback;
	backend->event_callback_data = window_data->event_callback_data;

	// initialize the event info data
	willis_wayland_reset_event_info(context);

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
		willis_wayland_reset_event_info(context);
	}

	willis_error_ok(error);
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
