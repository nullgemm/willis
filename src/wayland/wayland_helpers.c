#define _XOPEN_SOURCE 700
#include "include/willis.h"
#include "common/willis_private.h"
#include "wayland/wayland.h"
#include "wayland/wayland_helpers.h"
#include "nix/nix.h"

#include <linux/input.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include "zwp-relative-pointer-protocol.h"
#include "zwp-pointer-constraints-protocol.h"

// HACK
// we use this union to be able to shift the bits of a signed 64 bit integer
// in a portable way, as part of the conversion from wl_fixed_t to Q31.32
union i64_bits
{
	int64_t number;
	uint64_t bits;
};

// registry handler
void willis_wayland_helpers_registry_handler(
	void* data,
	struct wl_registry* registry,
	uint32_t name,
	const char* interface,
	uint32_t version)
{
	struct willis* context = data;
	struct wayland_backend* backend = context->backend_data;
	struct willis_error_info error;

	if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0)
	{
		backend->pointer_relative_manager =
			wl_registry_bind(
				registry,
				name,
				&zwp_relative_pointer_manager_v1_interface,
				1);

		if (backend->pointer_relative_manager == NULL)
		{
			willis_error_throw(
				context,
				&error,
				WILLIS_ERROR_WAYLAND_REQUEST);
			return;
		}
	}
	else if (strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0)
	{
		backend->pointer_constraints_manager =
			wl_registry_bind(
				registry,
				name,
				&zwp_pointer_constraints_v1_interface,
				1);

		if (backend->pointer_constraints_manager == NULL)
		{
			willis_error_throw(
				context,
				&error,
				WILLIS_ERROR_WAYLAND_REQUEST);

			return;
		}
	}
}

// capabilities handler
void willis_wayland_helpers_capabilities_handler(
	void* data,
	struct wl_seat* seat,
	uint32_t capabilities)
{
	struct willis* context = data;
	struct wayland_backend* backend = context->backend_data;

	struct willis_error_info error;
	int error_posix;

	bool pointer = (capabilities & WL_SEAT_CAPABILITY_POINTER) != 0;
	bool keyboard = (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0;

	if ((pointer == true) && (backend->pointer == NULL))
	{
		backend->pointer = wl_seat_get_pointer(seat);

		if (backend->pointer == NULL)
		{
			willis_error_throw(
				context,
				&error,
				WILLIS_ERROR_WAYLAND_POINTER_GET);

			return;
		}

		error_posix =
			wl_pointer_add_listener(
				backend->pointer,
				&(backend->listener_pointer),
				data);

		if (error_posix == -1)
		{
			willis_error_throw(
				context,
				&error,
				WILLIS_ERROR_WAYLAND_LISTENER_ADD);

			return;
		}
	}
	else if ((pointer == false) && (backend->pointer != NULL))
	{
		wl_pointer_release(backend->pointer);
		backend->pointer = NULL;
	}

	if ((keyboard == true) && (backend->keyboard == NULL))
	{
		backend->keyboard = wl_seat_get_keyboard(seat);

		if (backend->keyboard == NULL)
		{
			willis_error_throw(
				context,
				&error,
				WILLIS_ERROR_WAYLAND_KEYBOARD_GET);

			return;
		}

		error_posix =
			wl_keyboard_add_listener(
				backend->keyboard,
				&(backend->listener_keyboard),
				data);

		if (error_posix == -1)
		{
			willis_error_throw(
				context,
				&error,
				WILLIS_ERROR_WAYLAND_LISTENER_ADD);

			return;
		}
	}
	else if ((keyboard == false) && (backend->keyboard != NULL))
	{
		wl_keyboard_release(backend->keyboard);
		backend->keyboard = NULL;
	}
}

// event info reset
void willis_wayland_reset_event_info(
	struct willis* context)
{
	struct wayland_backend* backend = context->backend_data;

	struct willis_event_info event_info =
	{
		.event_code = WILLIS_NONE,
		.event_state = WILLIS_STATE_NONE,
		.utf8_string = NULL,
		.utf8_size = 0,
		.mouse_wheel_steps = 0,
		.mouse_x = 0,
		.mouse_y = 0,
		.diff_x = 0,
		.diff_y = 0,
	};

	backend->event_info = event_info;
}

// mouse coordinates format conversion
void willis_wayland_helpers_mouse(
	struct willis* context,
	wl_fixed_t x,
	wl_fixed_t y)
{
	struct wayland_backend* backend = context->backend_data;
	backend->event_info.mouse_x = wl_fixed_to_int(x);
	backend->event_info.mouse_y = wl_fixed_to_int(y);
	backend->event_info.event_code = WILLIS_MOUSE_MOTION;
	backend->event_info.event_state = WILLIS_STATE_NONE;
}

// pointer listeners
void willis_wayland_helpers_listener_pointer_enter(
	void* data,
	struct wl_pointer* pointer,
	uint32_t serial,
	struct wl_surface* surface,
	wl_fixed_t surface_x,
	wl_fixed_t surface_y)
{
	struct willis* context = data;
	struct wayland_backend* backend = context->backend_data;

	backend->event_serial = serial;
	backend->pointer_surface = surface;

	willis_wayland_helpers_mouse(data, surface_x, surface_y);

	backend->event_callback(backend->event_callback_data, &(backend->event_serial));
}

void willis_wayland_helpers_listener_pointer_leave(
	void* data,
	struct wl_pointer* pointer,
	uint32_t serial,
	struct wl_surface* surface)
{
	struct willis* context = data;
	struct wayland_backend* backend = context->backend_data;

	backend->event_serial = serial;

	if (backend->pointer_surface == surface)
	{
		backend->pointer_surface = NULL;
	}
}

void willis_wayland_helpers_listener_pointer_motion(
	void* data,
	struct wl_pointer* pointer,
	uint32_t time,
	wl_fixed_t surface_x,
	wl_fixed_t surface_y)
{
	struct willis* context = data;
	struct wayland_backend* backend = context->backend_data;

	willis_wayland_helpers_mouse(data, surface_x, surface_y);

	// use previous serial for this context since this event does not provide one
	backend->event_callback(backend->event_callback_data, &(backend->event_serial));
}

void willis_wayland_helpers_listener_pointer_button(
	void* data,
	struct wl_pointer* pointer,
	uint32_t serial,
	uint32_t time,
	uint32_t button,
	uint32_t state)
{
	struct willis* context = data;
	struct wayland_backend* backend = context->backend_data;
	backend->event_serial = serial;

	enum willis_event_code event_code;
	enum willis_event_state event_state;

	switch (button)
	{
		case BTN_LEFT:
		{
			event_code = WILLIS_MOUSE_CLICK_LEFT;
			break;
		}
		case BTN_RIGHT:
		{
			event_code = WILLIS_MOUSE_CLICK_RIGHT;
			break;
		}
		case BTN_MIDDLE:
		{
			event_code = WILLIS_MOUSE_CLICK_MIDDLE;
			break;
		}
		default:
		{
			event_code = WILLIS_NONE;
			break;
		}
	}

	if (state == WL_POINTER_BUTTON_STATE_RELEASED)
	{
		event_state = WILLIS_STATE_RELEASE;
	}
	else
	{
		event_state = WILLIS_STATE_PRESS;
	}

	backend->event_info.event_code = event_code;
	backend->event_info.event_state = event_state;

	backend->event_callback(backend->event_callback_data, &(backend->event_serial));
}

void willis_wayland_helpers_listener_pointer_axis_source(
	void* data,
	struct wl_pointer* pointer,
	uint32_t axis_source)
{
	// high-res axes are not supported by willis
}

void willis_wayland_helpers_listener_pointer_axis_stop(
	void* data,
	struct wl_pointer* pointer,
	uint32_t time,
	uint32_t axis)
{
	// high-res axes are not supported by willis
}

void willis_wayland_helpers_listener_pointer_axis_discrete(
	void* data,
	struct wl_pointer* pointer,
	uint32_t axis,
	int32_t discrete)
{
	// only regular mouse wheel is supported by willis
	if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
	{
		struct willis* context = data;
		struct wayland_backend* backend = context->backend_data;

		enum willis_event_code event_code;
		uint32_t max;

		if (discrete < 0)
		{
			event_code = WILLIS_MOUSE_WHEEL_UP;
			max = -discrete;
		}
		else
		{
			event_code = WILLIS_MOUSE_WHEEL_DOWN;
			max = discrete;
		}

		backend->event_info.event_code = event_code;
		backend->event_info.event_state = WILLIS_STATE_NONE;
		backend->event_info.mouse_wheel_steps = max;

		// use previous serial for this context since this event does not provide one
		backend->event_callback(backend->event_callback_data, &(backend->event_serial));
	}
}

void willis_wayland_helpers_listener_pointer_axis(
	void* data,
	struct wl_pointer* pointer,
	uint32_t time,
	uint32_t axis,
	wl_fixed_t value)
{
	// high-res axes are not supported by willis
	int32_t discrete;

	if (value > 0)
	{
		discrete = 1;
	}
	else
	{
		discrete = -1;
	}

	willis_wayland_helpers_listener_pointer_axis_discrete(
		data,
		pointer,
		axis,
		discrete);
}

void willis_wayland_helpers_listener_pointer_frame(
	void* data,
	struct wl_pointer* pointer)
{
	// high-res axes are not supported by willis
	// compositions are to be handled outside of willis
}

// keyboard listeners
void willis_wayland_helpers_listener_keyboard_keymap(
	void* data,
	struct wl_keyboard* keyboard,
	uint32_t format,
	int32_t fd,
	uint32_t size)
{
	struct willis* context = data;
	struct wayland_backend* backend = context->backend_data;

	if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
	{
		char* map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

		if (map_shm != MAP_FAILED)
		{
			if (backend->xkb_common->context == NULL)
			{
				// advanced keyboard handling
				willis_xkb_init_locale(backend->xkb_common);

				backend->xkb_common->context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

				if (backend->xkb_common->context == NULL)
				{
					return;
				}

				willis_xkb_init_compose(backend->xkb_common);
			}

			struct xkb_keymap* keymap =
				xkb_keymap_new_from_string(
					backend->xkb_common->context,
					map_shm,
					XKB_KEYMAP_FORMAT_TEXT_V1,
					XKB_KEYMAP_COMPILE_NO_FLAGS);

			munmap(map_shm, size);
			close(fd);

			if (keymap == NULL)
			{
				return;
			}

			struct xkb_state* state = xkb_state_new(keymap);

			if (state == NULL)
			{
				xkb_keymap_unref(keymap);
				return;
			}

			if (backend->xkb_common->keymap != NULL)
			{
				xkb_keymap_unref(backend->xkb_common->keymap);
			}

			if (backend->xkb_common->state != NULL)
			{
				xkb_state_unref(backend->xkb_common->state);
			}

			backend->xkb_common->keymap = keymap;
			backend->xkb_common->state = state;
		}
	}
}

void willis_wayland_helpers_listener_keyboard_enter(
	void* data,
	struct wl_keyboard* keyboard,
	uint32_t serial,
	struct wl_surface* surface,
	struct wl_array* keys)
{
	struct willis* context = data;
	struct wayland_backend* backend = context->backend_data;
	backend->event_serial = serial;

	struct willis_error_info error;
	uint32_t* key;

	willis_error_ok(&error);

	if (backend->xkb_common->compose_state != NULL)
	{
		wl_array_for_each(key, keys)
		{
			backend->event_info.event_code = willis_xkb_translate_keycode(*key + 8);
			backend->event_info.event_state = WILLIS_STATE_PRESS;

			willis_xkb_utf8_compose(
				context,
				backend->xkb_common,
				*key + 8,
				&(backend->event_info.utf8_string),
				&(backend->event_info.utf8_size),
				&error);

			if (willis_error_get_code(&error) == WILLIS_ERROR_OK)
			{
				backend->event_callback(
					backend->event_callback_data,
					&(backend->event_serial));
			}
		}
	}
	else
	{
		wl_array_for_each(key, keys)
		{
			backend->event_info.event_code = willis_xkb_translate_keycode(*key + 8);
			backend->event_info.event_state = WILLIS_STATE_PRESS;

			willis_xkb_utf8_simple(
				context,
				backend->xkb_common,
				*key + 8,
				&(backend->event_info.utf8_string),
				&(backend->event_info.utf8_size),
				&error);

			if (willis_error_get_code(&error) == WILLIS_ERROR_OK)
			{
				backend->event_callback(
					backend->event_callback_data,
					&(backend->event_serial));
			}
		}
	}
}

void willis_wayland_helpers_listener_keyboard_leave(
	void* data,
	struct wl_keyboard* keyboard,
	uint32_t serial,
	struct wl_surface* surface)
{
	// not needed
}

void willis_wayland_helpers_listener_keyboard_key(
	void* data,
	struct wl_keyboard* keyboard,
	uint32_t serial,
	uint32_t time,
	uint32_t key,
	uint32_t state)
{
	struct willis* context = data;
	struct wayland_backend* backend = context->backend_data;

	struct willis_error_info error;
	backend->event_serial = serial;
	key += 8;

	willis_error_ok(&error);

	backend->event_info.event_code = willis_xkb_translate_keycode(key);

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
	{
		backend->event_info.event_state = WILLIS_STATE_PRESS;

		if (backend->xkb_common->compose_state != NULL)
		{
			willis_xkb_utf8_compose(
				context,
				backend->xkb_common,
				key,
				&(backend->event_info.utf8_string),
				&(backend->event_info.utf8_size),
				&error);
		}
		else
		{
			willis_xkb_utf8_simple(
				context,
				backend->xkb_common,
				key,
				&(backend->event_info.utf8_string),
				&(backend->event_info.utf8_size),
				&error);
		}
	}
	else
	{
		backend->event_info.event_state = WILLIS_STATE_RELEASE;
	}

	if (willis_error_get_code(&error) == WILLIS_ERROR_OK)
	{
		backend->event_callback(backend->event_callback_data, &(backend->event_serial));
	}
}

void willis_wayland_helpers_listener_keyboard_modifiers(
	void* data,
	struct wl_keyboard* keyboard,
	uint32_t serial,
	uint32_t mods_depressed,
	uint32_t mods_latched,
	uint32_t mods_locked,
	uint32_t group)
{
	struct willis* context = data;
	struct wayland_backend* backend = context->backend_data;
	backend->event_serial = serial;

	if (backend->xkb_common->state != NULL)
	{
		xkb_state_update_mask(
			backend->xkb_common->state,
			mods_depressed,
			mods_latched,
			mods_locked,
			0,
			0,
			group);
	}

	backend->event_callback(backend->event_callback_data, &(backend->event_serial));
}

void willis_wayland_helpers_listener_keyboard_repeat_info(
	void* data,
	struct wl_keyboard* keyboard,
	int32_t rate,
	int32_t delay)
{
	// not needed
}

void willis_wayland_helpers_listener_pointer_relative(
	void* data,
	struct zwp_relative_pointer_v1* pointer,
	uint32_t time_msp,
	uint32_t time_lsp,
	wl_fixed_t x,
	wl_fixed_t y,
	wl_fixed_t x_linear,
	wl_fixed_t y_linear)
{
	struct willis* context = data;
	struct wayland_backend* backend = context->backend_data;

	union i64_bits convert;
	convert.number = x_linear;
	backend->event_info.diff_x = convert.bits << 24;
	convert.number = y_linear;
	backend->event_info.diff_y = convert.bits << 24;

	backend->event_info.event_code = WILLIS_MOUSE_MOTION;
	backend->event_info.event_state = WILLIS_STATE_NONE;

	// use previous serial for this context since this event does not provide one
	backend->event_callback(backend->event_callback_data, &(backend->event_serial));
}

void willis_wayland_helpers_listener_pointer_locked(
	void* data,
	struct zwp_locked_pointer_v1* locked)
{
	// not needed
}

void willis_wayland_helpers_listener_pointer_unlocked(
	void* data,
	struct zwp_locked_pointer_v1* locked)
{
	// not needed
}
