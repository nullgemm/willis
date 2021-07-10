#ifndef H_WILLIS_INTERNAL_WAYLAND
#define H_WILLIS_INTERNAL_WAYLAND

#include "zwp-pointer-constraints-protocol.h"
#include "zwp-relative-pointer-protocol.h"

#include <stdint.h>
#include <pthread.h>
#include <wayland-client.h>

struct willis_data_wayland
{
	struct zwp_relative_pointer_manager_v1* wl_relative_pointer;
	struct zwp_pointer_constraints_v1* wl_pointer_constraints;
	struct wl_surface* wl_surface;
	void (*callback_serial)(void*, uint32_t);
	void* callback_serial_data;
};

struct willis_wayland
{
	struct zwp_relative_pointer_v1_listener wl_pointer_relative_listener;
	struct zwp_locked_pointer_v1_listener wl_pointer_locked_listener;

	struct zwp_relative_pointer_manager_v1* wl_pointer_relative_manager;
	struct zwp_pointer_constraints_v1* wl_pointer_constraints_manager;

	struct zwp_relative_pointer_v1* wl_pointer_relative;
	struct zwp_locked_pointer_v1* wl_pointer_locked;

	struct wl_seat_listener wl_seat_listener;
	struct wl_pointer_listener wl_pointer_listener;
	struct wl_keyboard_listener wl_keyboard_listener;

	struct wl_seat* wl_seat;
	struct wl_pointer* wl_pointer;
	struct wl_keyboard* wl_keyboard;
	struct wl_surface* wl_surface;

	void (*callback_serial)(void*, uint32_t);
	void* callback_serial_data;

	pthread_mutex_t mutex;
};

#endif
