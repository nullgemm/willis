#define _XOPEN_SOURCE 700

#include "willis.h"
#include "willis_events.h"
#include "willis_wayland.h"
#include "willis_xkb.h"
#include "xkb.h"
#include "zwp-relative-pointer-protocol.h"
#include "zwp-pointer-constraints-protocol.h"

#include <linux/input-event-codes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

// Mandatory Dirty Hack ©
// Bear with me for a second...
//
// Dodging the synchronization issues under wayland is hard because
// there are two points of potential initialization we must take into account.
//
// To make willis support multiple contexts we initialize intance mutexes
// in advance and copy them in the appropriate instance when needed: this should
// be thread-safe enough because we only overwrite the same mutex data at worst.
//
// Once we are able to synchronize it is easy to count the initialization passes
// and reset the global mutex for the next potential context to use.
//
// Of course this means willis contexts can only be initialized sequentially,
// so context initialization itself is not thread-safe, and cannot be made so.
static pthread_mutex_t global_willis_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned int global_willis_mutex_passes = 0;

// standard enough
union willis_convert
{
	int64_t number;
	uint64_t bits;
};

void wl_seat_capabilities(
	void* data,
	struct wl_seat* wl_seat,
	uint32_t capabilities)
{
	struct willis* willis = data;
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);

	bool pointer = (capabilities & WL_SEAT_CAPABILITY_POINTER) != 0;
	bool keyboard = (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0;

	pthread_mutex_lock(&(willis_wayland->mutex));

	if ((pointer == true) && (willis_wayland->wl_pointer == NULL))
	{
		willis_wayland->wl_pointer = wl_seat_get_pointer(willis_wayland->wl_seat);

		wl_pointer_add_listener(
			willis_wayland->wl_pointer,
			&(willis_wayland->wl_pointer_listener),
			willis);
	}
	else if ((pointer == false) && (willis_wayland->wl_pointer != NULL))
	{
		wl_pointer_release(willis_wayland->wl_pointer);

		willis_wayland->wl_pointer = NULL;
	}

	if ((keyboard == true) && (willis_wayland->wl_keyboard == NULL))
	{
		willis_wayland->wl_keyboard = wl_seat_get_keyboard(willis_wayland->wl_seat);

		wl_keyboard_add_listener(
			willis_wayland->wl_keyboard,
			&(willis_wayland->wl_keyboard_listener),
			willis);
	}
	else if ((keyboard == false) && (willis_wayland->wl_keyboard != NULL))
	{
		wl_keyboard_release(willis_wayland->wl_keyboard);

		willis_wayland->wl_keyboard = NULL;
	}

	pthread_mutex_unlock(&(willis_wayland->mutex));
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
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);
	pthread_mutex_lock(&(willis_wayland->mutex));

	willis->mouse_x = wl_fixed_to_int(x);
	willis->mouse_y = wl_fixed_to_int(y);

	willis->callback(
		willis,
		WILLIS_MOUSE_MOTION,
		WILLIS_STATE_NONE,
		willis->data);

	pthread_mutex_unlock(&(willis_wayland->mutex));
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
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);
	enum willis_event_code event_code;
	enum willis_event_state event_state;

	pthread_mutex_lock(&(willis_wayland->mutex));

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

	pthread_mutex_unlock(&(willis_wayland->mutex));
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
		struct willis_wayland* willis_wayland = &(willis->willis_wayland);
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

		pthread_mutex_lock(&(willis_wayland->mutex));

		for (uint32_t i = 0; i < max; ++i)
		{
			willis->callback(
				willis,
				event_code,
				WILLIS_STATE_NONE,
				willis->data);
		}

		pthread_mutex_unlock(&(willis_wayland->mutex));
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
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);

	pthread_mutex_lock(&(willis_wayland->mutex));

	struct willis_xkb* willis_xkb = &(willis->willis_xkb);

	if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
	{
		char* map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

		if (map_shm != MAP_FAILED)
		{
			if (willis_xkb->xkb_ctx == NULL)
			{
				// advanced keyboard handling
				willis_xkb_init_locale(willis);

				willis_xkb->xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

				if (willis_xkb->xkb_ctx == NULL)
				{
					pthread_mutex_unlock(&(willis_wayland->mutex));
					return;
				}

				willis_xkb_init_compose(willis);
			}

			struct xkb_keymap* keymap =
				xkb_keymap_new_from_string(
					willis_xkb->xkb_ctx,
					map_shm,
					XKB_KEYMAP_FORMAT_TEXT_V1,
					XKB_KEYMAP_COMPILE_NO_FLAGS);

			munmap(map_shm, size);
			close(fd);

			if (keymap == NULL)
			{
				pthread_mutex_unlock(&(willis_wayland->mutex));
				return;
			}

			struct xkb_state* state = xkb_state_new(keymap);

			if (state == NULL)
			{
				xkb_keymap_unref(keymap);
				pthread_mutex_unlock(&(willis_wayland->mutex));
				return;
			}

			if (willis_xkb->xkb_keymap != NULL)
			{
				xkb_keymap_unref(willis_xkb->xkb_keymap);
			}

			if (willis_xkb->xkb_state != NULL)
			{
				xkb_state_unref(willis_xkb->xkb_state);
			}

			willis_xkb->xkb_keymap = keymap;
			willis_xkb->xkb_state = state;
		}
	}

	pthread_mutex_unlock(&(willis_wayland->mutex));
}

static void wl_keyboard_enter(
	void* data,
	struct wl_keyboard* wl_keyboard,
	uint32_t serial,
	struct wl_surface* surface,
	struct wl_array* keys)
{
	struct willis* willis = data;
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);

	pthread_mutex_lock(&(willis_wayland->mutex));

	struct willis_xkb* willis_xkb = &(willis->willis_xkb);
	uint32_t* key;

	if (willis->get_utf8 == true)
	{
		if (willis_xkb->xkb_compose_state != NULL)
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

	pthread_mutex_unlock(&(willis_wayland->mutex));
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
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);

	pthread_mutex_lock(&(willis_wayland->mutex));

	struct willis_xkb* willis_xkb = &(willis->willis_xkb);
	enum willis_event_code event_code;
	enum willis_event_state event_state;

	key += 8;

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
	{
		if (willis->get_utf8 == true)
		{
			if (willis_xkb->xkb_compose_state != NULL)
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

	pthread_mutex_unlock(&(willis_wayland->mutex));
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
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);

	pthread_mutex_lock(&(willis_wayland->mutex));

	struct willis_xkb* willis_xkb = &(willis->willis_xkb);

	if (willis_xkb->xkb_state != NULL)
	{
		xkb_state_update_mask(
			willis_xkb->xkb_state,
			mods_depressed,
			mods_latched,
			mods_locked,
			0,
			0,
			group);
	}

	pthread_mutex_unlock(&(willis_wayland->mutex));
}

static void wl_keyboard_repeat_info()
{

}

static void relative_listener(
	void* data,
	struct zwp_relative_pointer_v1* pointer,
	uint32_t time_msp,
	uint32_t time_lsp,
	wl_fixed_t x,
	wl_fixed_t y,
	wl_fixed_t x_linear,
	wl_fixed_t y_linear)
{
	struct willis* willis = (void*) data;
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);

	pthread_mutex_lock(&(willis_wayland->mutex));

	union willis_convert convert;

	convert.number = x_linear;
	willis->diff_x = convert.bits << 24;

	convert.number = y_linear;
	willis->diff_y = convert.bits << 24;

	willis->callback(
		willis,
		WILLIS_MOUSE_MOTION,
		WILLIS_STATE_NONE,
		willis->data);

	pthread_mutex_unlock(&(willis_wayland->mutex));
}

static void locked_listener(
	void* data,
	struct zwp_locked_pointer_v1* locked)
{

}

static void unlocked_listener(
	void* data,
	struct zwp_locked_pointer_v1* locked)
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
	int ok;
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);
	struct willis_data_wayland* willis_data = (void*) backend_link;

	ok = pthread_mutex_lock(&global_willis_mutex);

	if (ok != 0)
	{
		return false;
	}

	++global_willis_mutex_passes;

	if (global_willis_mutex_passes < 2)
	{
		ok = pthread_mutex_init(&(willis_wayland->mutex), NULL);

		if (ok != 0)
		{
			return false;
		}
	}

	ok = pthread_mutex_lock(&(willis_wayland->mutex));

	if (ok != 0)
	{
		return false;
	}

	ok = pthread_mutex_unlock(&global_willis_mutex);

	if (ok != 0)
	{
		pthread_mutex_unlock(&(willis_wayland->mutex));
		return false;
	}

	willis_wayland->wl_pointer_relative_manager =
		willis_data->wl_relative_pointer;
	willis_wayland->wl_pointer_constraints_manager =
		willis_data->wl_pointer_constraints;
	willis_wayland->wl_surface =
		willis_data->wl_surface;

	struct zwp_relative_pointer_v1_listener relative =
	{
		.relative_motion = relative_listener,
	};

	struct zwp_locked_pointer_v1_listener locked =
	{
		.locked = locked_listener,
		.unlocked = unlocked_listener,
	};

	willis_wayland->wl_pointer_locked_listener = locked;
	willis_wayland->wl_pointer_relative_listener = relative;

	// common init
	willis->callback = callback; // lol what is synchronization
	willis->data = data;
	willis->get_utf8 = utf8;
	willis->utf8_string = NULL;
	willis->utf8_size = 0;
	willis->mouse_grab = false;
	willis->mouse_x = 0;
	willis->mouse_y = 0;
	willis->diff_x = 0;
	willis->diff_y = 0;

	ok = pthread_mutex_unlock(&(willis_wayland->mutex));

	if (ok != 0)
	{
		return false;
	}

	// Wait for the event callback initializer to be called before returning.
	// This mechanism is provided as a courtesy to the developer to help the
	// sequential initialization of multiple willis contexts, if needed.
	unsigned int passes;

	do
	{
		ok = pthread_mutex_lock(&global_willis_mutex);

		if (ok != 0)
		{
			return false;
		}

		passes = global_willis_mutex_passes;

		ok = pthread_mutex_unlock(&global_willis_mutex);

		if (ok != 0)
		{
			return false;
		}
	}
	while(passes < 2);

	global_willis_mutex_passes = 0;

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
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);

	pthread_mutex_lock(&global_willis_mutex);

	++global_willis_mutex_passes;

	if (global_willis_mutex_passes < 2)
	{
		pthread_mutex_init(&(willis_wayland->mutex), NULL);
	}

	pthread_mutex_lock(&(willis_wayland->mutex));
	pthread_mutex_unlock(&global_willis_mutex);

	// we must initialize these now to prevent the apocalypse
	// in case events are processed before willis_init is called
	// (which *will* happen if your computer is not a potato)
	willis_wayland->wl_seat = event;
	willis->callback = dummy_callback;

	// initialize required structures
	struct wl_seat_listener seat_listener =
	{
		.capabilities = wl_seat_capabilities,
		.name = wl_seat_name,
	};

	willis_wayland->wl_seat_listener = seat_listener;

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

	willis_wayland->wl_pointer_listener = pointer_listener;

	struct wl_keyboard_listener keyboard_listener =
	{
		.keymap = wl_keyboard_keymap,
		.enter = wl_keyboard_enter,
		.leave = wl_keyboard_leave,
		.key = wl_keyboard_key,
		.modifiers = wl_keyboard_modifiers,
		.repeat_info = wl_keyboard_repeat_info,
	};

	willis_wayland->wl_keyboard_listener = keyboard_listener;

	wl_seat_add_listener(event, &(willis_wayland->wl_seat_listener), willis);

	pthread_mutex_unlock(&(willis_wayland->mutex));
}

bool willis_free(struct willis* willis)
{
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);
	struct willis_xkb* willis_xkb = &(willis->willis_xkb);

	if (willis_xkb->xkb_state != NULL)
	{
		xkb_state_unref(willis_xkb->xkb_state);
	}

	if (willis_xkb->xkb_keymap != NULL)
	{
		xkb_keymap_unref(willis_xkb->xkb_keymap);
	}

	if (willis_xkb->xkb_compose_table != NULL)
	{
		xkb_compose_table_unref(willis_xkb->xkb_compose_table);
	}

	if (willis_xkb->xkb_compose_state != NULL)
	{
		xkb_compose_state_unref(willis_xkb->xkb_compose_state);
	}

	if (willis_xkb->xkb_ctx != NULL)
	{
		xkb_context_unref(willis_xkb->xkb_ctx);
	}

	if (willis_wayland->wl_seat != NULL)
	{
		wl_seat_release(willis_wayland->wl_seat);
	}

	if (willis_wayland->wl_pointer != NULL)
	{
		wl_pointer_release(willis_wayland->wl_pointer);
	}

	if (willis_wayland->wl_keyboard != NULL)
	{
		wl_keyboard_release(willis_wayland->wl_keyboard);
	}

	return true;
}

// must be called from the event callback
bool willis_mouse_grab(struct willis* willis)
{
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);

	if (willis->mouse_grab == true)
	{
		return false;
	}

	if (willis_wayland->wl_pointer_relative_manager == NULL)
	{
		return false;
	}

	wl_pointer_set_cursor(
		willis_wayland->wl_pointer,
		0,
		NULL,
		0,
		0);

	willis_wayland->wl_pointer_relative =
		zwp_relative_pointer_manager_v1_get_relative_pointer(
			willis_wayland->wl_pointer_relative_manager,
			willis_wayland->wl_pointer);

	zwp_relative_pointer_v1_add_listener(
		willis_wayland->wl_pointer_relative,
		&willis_wayland->wl_pointer_relative_listener,
		willis);

	willis_wayland->wl_pointer_locked =
		zwp_pointer_constraints_v1_lock_pointer(
			willis_wayland->wl_pointer_constraints_manager,
			willis_wayland->wl_surface,
			willis_wayland->wl_pointer,
			NULL,
			ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);

	zwp_locked_pointer_v1_add_listener(
		willis_wayland->wl_pointer_locked,
		&willis_wayland->wl_pointer_locked_listener,
		willis);

	willis->mouse_grab = true;

	return true;
}

// must be called from the event callback
bool willis_mouse_ungrab(struct willis* willis)
{
	struct willis_wayland* willis_wayland = &(willis->willis_wayland);

	if (willis->mouse_grab == false)
	{
		return false;
	}

	zwp_relative_pointer_v1_destroy(willis_wayland->wl_pointer_relative);
	zwp_locked_pointer_v1_destroy(willis_wayland->wl_pointer_locked);

	willis_wayland->wl_pointer_relative = NULL;
	willis_wayland->wl_pointer_locked = NULL;

	willis->mouse_grab = false;

	return true;
}
