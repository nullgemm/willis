#ifndef H_WILLIS_INTERNAL_WAYLAND
#define H_WILLIS_INTERNAL_WAYLAND

#include "willis.h"
#include "common/willis_error.h"
#include "nix/nix.h"

#include <stdint.h>
#include <stdbool.h>
#include <wayland-client.h>
#include "zwp-relative-pointer-protocol.h"
#include "zwp-pointer-constraints-protocol.h"

struct wayland_backend
{
	bool mouse_grabbed;
	struct willis_xkb* xkb_common;

	// event storage
	uint32_t event_serial;
	struct willis_event_info event_info;

	// event callback
	void (*event_callback)(
		void* data,
		void* event);
	void* event_callback_data;

	// core structures
	struct wl_pointer* pointer;
	struct wl_keyboard* keyboard;
	struct wl_surface* pointer_surface;

	// pointer structures
	struct zwp_relative_pointer_v1* pointer_relative;
	struct zwp_locked_pointer_v1* pointer_locked;
	struct zwp_relative_pointer_manager_v1* pointer_relative_manager;
	struct zwp_pointer_constraints_v1* pointer_constraints_manager;

	// listeners
	struct wl_pointer_listener listener_pointer;
	struct wl_keyboard_listener listener_keyboard;
	struct zwp_relative_pointer_v1_listener listener_pointer_relative;
	struct zwp_locked_pointer_v1_listener listener_pointer_locked;
};

void willis_wayland_init(
	struct willis* context,
	struct willis_error_info* error);

void willis_wayland_start(
	struct willis* context,
	void* data,
	struct willis_error_info* error);

void willis_wayland_handle_event(
	struct willis* context,
	void* event,
	struct willis_event_info* event_info,
	struct willis_error_info* error);

bool willis_wayland_mouse_grab(
	struct willis* context,
	struct willis_error_info* error);

bool willis_wayland_mouse_ungrab(
	struct willis* context,
	struct willis_error_info* error);

void willis_wayland_stop(
	struct willis* context,
	struct willis_error_info* error);

void willis_wayland_clean(
	struct willis* context,
	struct willis_error_info* error);

#endif
