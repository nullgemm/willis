#define _XOPEN_SOURCE 700
#include "willis.h"

#include "willis_events.h"
#include "xkb.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <linux/input-event-codes.h>

void wl_seat_capabilities(
	void* data,
	struct wl_seat* wl_seat,
	uint32_t capabilities)
{
	struct willis* willis = data;
	bool pointer = (capabilities & WL_SEAT_CAPABILITY_POINTER) != 0;
	bool keyboard = (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0;

	if ((pointer == true) && (willis->wl_pointer == NULL))
	{
		willis->wl_pointer = wl_seat_get_pointer(willis->wl_seat);

		wl_pointer_add_listener(
			willis->wl_pointer,
			&(willis->wl_pointer_listener),
			willis);
	}
	else if ((pointer == false) && (willis->wl_pointer != NULL))
	{
		wl_pointer_release(willis->wl_pointer);

		willis->wl_pointer = NULL;
	}

	if ((keyboard == true) && (willis->wl_keyboard == NULL))
	{
		willis->wl_keyboard = wl_seat_get_keyboard(willis->wl_seat);

		wl_keyboard_add_listener(
			willis->wl_keyboard,
			&(willis->wl_keyboard_listener),
			willis);
	}
	else if ((keyboard == false) && (willis->wl_keyboard != NULL))
	{
		wl_keyboard_release(willis->wl_keyboard);

		willis->wl_keyboard = NULL;
	}
}

void wl_seat_name(
	void* data,
	struct wl_seat* wl_seat,
	const char* name)
{

}

// from here, static functions are willis factorization helpers
static inline void mouse(struct willis* willis, wl_fixed_t x, wl_fixed_t y)
{
	willis->mouse_x = wl_fixed_to_int(x);
	willis->mouse_y = wl_fixed_to_int(y);

	willis->callback(
		willis,
		WILLIS_MOUSE_MOTION,
		WILLIS_STATE_NONE,
		willis->data);
}

// from here, static functions are wayland pointer callbacks
// TODO support serials ?
static void wl_pointer_enter(
	void* data,
	struct wl_pointer* wl_pointer,
	uint32_t serial,
	struct wl_surface* surface,
	wl_fixed_t surface_x,
	wl_fixed_t surface_y)
{
	mouse(data, surface_x, surface_y);
}

static void wl_pointer_leave(
	void* data,
	struct wl_pointer* wl_pointer,
	uint32_t serial,
	struct wl_surface* surface)
{

}

static void wl_pointer_motion(
	void* data,
	struct wl_pointer* wl_pointer,
	uint32_t time,
	wl_fixed_t surface_x,
	wl_fixed_t surface_y)
{
	mouse(data, surface_x, surface_y);
}

static void wl_pointer_button(
	void* data,
	struct wl_pointer* wl_pointer,
	uint32_t serial,
	uint32_t time,
	uint32_t button,
	uint32_t state)
{
	struct willis* willis = data;
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

	willis->callback(
		willis,
		event_code,
		event_state,
		willis->data);
}

static void wl_pointer_axis(
	void* data,
	struct wl_pointer* wl_pointer,
	uint32_t time,
	uint32_t axis,
	wl_fixed_t value)
{
	// high-res axes are not supported by willis
}

static void wl_pointer_axis_source(
	void* data,
	struct wl_pointer* wl_pointer,
	uint32_t axis_source)
{
	// high-res axes are not supported by willis
}

static void wl_pointer_axis_stop(
	void* data,
	struct wl_pointer* wl_pointer,
	uint32_t time,
	uint32_t axis)
{
	// high-res axes are not supported by willis
}

static void wl_pointer_axis_discrete(
	void* data,
	struct wl_pointer* wl_pointer,
	uint32_t axis,
	int32_t discrete)
{
	// only regular mouse wheel is supported by willis
	if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
	{
		enum willis_event_code event_code;
		struct willis* willis = data;
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

		for (uint32_t i = 0; i < max; ++i)
		{
			willis->callback(
				willis,
				event_code,
				WILLIS_STATE_NONE,
				willis->data);
		}
	}
}

static void wl_pointer_frame()
{
	// high-res axes are not supported by willis
	// compositions are to be hanlded outside of willis
}

// from here, static functions are wayland keyboard callbacks

static void wl_keyboard_keymap(
	void* data,
	struct wl_keyboard* wl_keyboard,
	uint32_t format,
	int32_t fd,
	uint32_t size)
{
	struct willis* willis = data;

	if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
	{
		char* map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

		if (map_shm != MAP_FAILED)
		{
			struct xkb_keymap* keymap =
				xkb_keymap_new_from_string(
					willis->xkb_ctx,
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

			if (willis->xkb_keymap != NULL)
			{
				xkb_keymap_unref(willis->xkb_keymap);
			}

			if (willis->xkb_state != NULL)
			{
				xkb_state_unref(willis->xkb_state);
			}

			willis->xkb_keymap = keymap;
			willis->xkb_state = state;
		}
	}
}

static void wl_keyboard_enter(
	void* data,
	struct wl_keyboard* wl_keyboard,
	uint32_t serial,
	struct wl_surface* surface,
	struct wl_array* keys)
{
	struct willis* willis = data;
	uint32_t* key;

	if (willis->get_utf8 == true)
	{
		if (willis->xkb_compose_state != NULL)
		{
			wl_array_for_each(key, keys)
			{
				willis_utf8_compose(willis, *key + 8);
				willis->callback(
					willis,
					willis_translate_keycode_x11(*key + 8),
					WILLIS_STATE_PRESS,
					willis->data);
			}
		}
		else
		{
			wl_array_for_each(key, keys)
			{
				willis_utf8_simple(willis, *key + 8);
				willis->callback(
					willis,
					willis_translate_keycode_x11(*key + 8),
					WILLIS_STATE_PRESS,
					willis->data);
			}
		}
	}
	else
	{
		wl_array_for_each(key, keys)
		{
			willis->callback(
				willis,
				willis_translate_keycode_x11(*key + 8),
				WILLIS_STATE_PRESS,
				willis->data);
		}
	}
}

static void wl_keyboard_leave(
	void* data,
	struct wl_keyboard* wl_keyboard,
	uint32_t serial,
	struct wl_surface* surface)
{

}

static void wl_keyboard_key(
	void* data,
	struct wl_keyboard* wl_keyboard,
	uint32_t serial,
	uint32_t time,
	uint32_t key,
	uint32_t state)
{
	struct willis* willis = data;
	enum willis_event_code event_code;
	enum willis_event_state event_state;

	key += 8;

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
	{
		if (willis->get_utf8 == true)
		{
			if (willis->xkb_compose_state != NULL)
			{
				willis_utf8_compose(willis, key);
			}
			else
			{
				willis_utf8_simple(willis, key);
			}
		}

		event_state = WILLIS_STATE_PRESS;
	}
	else
	{
		event_state = WILLIS_STATE_RELEASE;
	}

	event_code = willis_translate_keycode_x11(key);

	willis->callback(willis, event_code, event_state, willis->data);

	if (willis->utf8_string != NULL)
	{
		free(willis->utf8_string);
		willis->utf8_string = NULL;
		willis->utf8_size = 0;
	}
}

static void wl_keyboard_modifiers(
	void* data,
	struct wl_keyboard* wl_keyboard,
	uint32_t serial,
	uint32_t mods_depressed,
	uint32_t mods_latched,
	uint32_t mods_locked,
	uint32_t group)
{
	struct willis* willis = data;

	xkb_state_update_mask(
		willis->xkb_state,
		mods_depressed,
		mods_latched,
		mods_locked,
		0,
		0,
		group);
}

static void wl_keyboard_repeat_info()
{

}

bool willis_init(
	struct willis* willis,
	void* backend_link,
	bool utf8,
	void (*callback)(
		struct willis* willis,
		enum willis_event_code event_code,
		enum willis_event_state event_state,
		void* data),
	void* data)
{
	willis_xkb_init_locale(willis);

	willis->xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	if (willis->xkb_ctx == NULL)
	{
		return false;
	}

	willis_xkb_init_compose(willis);

	willis->callback = callback; // lol what is synchronization
	willis->data = data;
	willis->utf8_string = NULL;
	willis->utf8_size = 0;
	willis->get_utf8 = utf8;

	return true;
}

// dummy callback
static void dummy_callback(
	struct willis* willis,
	enum willis_event_code event_code,
	enum willis_event_state event_state,
	void* data)
{

}

void willis_handle_events(
	void* event,
	void* ctx)
{
	struct willis* willis = ctx;

	// we must initialize these now to prevent the apocalypse
	// in case events are processed before willis_init is called
	// (which *will* happen if your computer is not a potato)
	willis->wl_seat = event;
	willis->callback = dummy_callback;
	willis->data = NULL;
	willis->utf8_string = NULL;
	willis->utf8_size = 0;
	willis->get_utf8 = false;

	willis->wl_pointer = NULL;
	willis->wl_keyboard = NULL;
	willis->wl_touch = NULL;

	willis->xkb_keymap = NULL;
	willis->xkb_state = NULL;

	// initialize required structures
	struct wl_seat_listener seat_listener =
	{
		.capabilities = wl_seat_capabilities,
		.name = wl_seat_name,
	};

	willis->wl_seat_listener = seat_listener;

	struct wl_pointer_listener pointer_listener =
	{
		.enter = wl_pointer_enter,
		.leave = wl_pointer_leave,
		.motion = wl_pointer_motion,
		.button = wl_pointer_button,
		.axis = wl_pointer_axis,
		.frame = wl_pointer_frame,
		.axis_source = wl_pointer_axis_source,
		.axis_stop = wl_pointer_axis_stop,
		.axis_discrete = wl_pointer_axis_discrete,
	};

	willis->wl_pointer_listener = pointer_listener;

	struct wl_keyboard_listener keyboard_listener =
	{
		.keymap = wl_keyboard_keymap,
		.enter = wl_keyboard_enter,
		.leave = wl_keyboard_leave,
		.key = wl_keyboard_key,
		.modifiers = wl_keyboard_modifiers,
		.repeat_info = wl_keyboard_repeat_info,
	};

	willis->wl_keyboard_listener = keyboard_listener;

	wl_seat_add_listener(event, &(willis->wl_seat_listener), willis);
}
