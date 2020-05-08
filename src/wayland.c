#include "willis.h"
#include "willis_events.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <wayland-client.h>
#include <linux/input-event-codes.h>

void wl_seat_capabilities(
	void* data,
	struct wl_seat* wl_seat,
	uint32_t capabilities)
{
	struct willis* willis = data;
	bool pointer = (capabilities & WL_SEAT_CAPABILITY_POINTER) != 0;

	if ((pointer == true) && (willis->wl_pointer == NULL))
	{
		willis->wl_pointer = wl_seat_get_pointer(willis->wl_seat);

		wl_pointer_add_listener(
			willis->wl_pointer,
			&willis->wl_pointer_listener,
			willis);
	}
	else if ((pointer == false) && (willis->wl_pointer != NULL))
	{
		wl_pointer_release(willis->wl_pointer);

		willis->wl_pointer = NULL;
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

// from here, static functions are wayland callbacks
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

	wl_seat_add_listener(event, &(willis->wl_seat_listener), willis);
}
