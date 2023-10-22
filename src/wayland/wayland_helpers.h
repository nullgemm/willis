#ifndef H_WILLIS_INTERNAL_WAYLAND_HELPERS
#define H_WILLIS_INTERNAL_WAYLAND_HELPERS

#include "include/willis.h"
#include "wayland/wayland.h"
#include <wayland-client.h>
#include "zwp-relative-pointer-protocol.h"
#include "zwp-pointer-constraints-protocol.h"

// registry handler
void willis_wayland_helpers_registry_handler(
	void* data,
	struct wl_registry* registry,
	uint32_t name,
	const char* interface,
	uint32_t version);

// capabilities handler
void willis_wayland_helpers_capabilities_handler(
	void* data,
	struct wl_seat* seat,
	uint32_t capabilities);

// event info reset
void willis_wayland_reset_event_info(
	struct willis* context);

// mouse coordinates format conversion
void willis_wayland_helpers_mouse(
	struct willis* context,
	wl_fixed_t x,
	wl_fixed_t y);

// pointer listeners
void willis_wayland_helpers_listener_pointer_enter(
	void* data,
	struct wl_pointer* pointer,
	uint32_t serial,
	struct wl_surface* surface,
	wl_fixed_t surface_x,
	wl_fixed_t surface_y);

void willis_wayland_helpers_listener_pointer_leave(
	void* data,
	struct wl_pointer* pointer,
	uint32_t serial,
	struct wl_surface* surface);

void willis_wayland_helpers_listener_pointer_motion(
	void* data,
	struct wl_pointer* pointer,
	uint32_t time,
	wl_fixed_t surface_x,
	wl_fixed_t surface_y);

void willis_wayland_helpers_listener_pointer_button(
	void* data,
	struct wl_pointer* pointer,
	uint32_t serial,
	uint32_t time,
	uint32_t button,
	uint32_t state);

void willis_wayland_helpers_listener_pointer_axis_source(
	void* data,
	struct wl_pointer* pointer,
	uint32_t axis_source);

void willis_wayland_helpers_listener_pointer_axis_stop(
	void* data,
	struct wl_pointer* pointer,
	uint32_t time,
	uint32_t axis);

void willis_wayland_helpers_listener_pointer_axis_discrete(
	void* data,
	struct wl_pointer* pointer,
	uint32_t axis,
	int32_t discrete);

void willis_wayland_helpers_listener_pointer_axis(
	void* data,
	struct wl_pointer* pointer,
	uint32_t time,
	uint32_t axis,
	wl_fixed_t value);

void willis_wayland_helpers_listener_pointer_frame(
	void* data,
	struct wl_pointer* pointer);

// keyboard listeners
void willis_wayland_helpers_listener_keyboard_keymap(
	void* data,
	struct wl_keyboard* keyboard,
	uint32_t format,
	int32_t fd,
	uint32_t size);

void willis_wayland_helpers_listener_keyboard_enter(
	void* data,
	struct wl_keyboard* keyboard,
	uint32_t serial,
	struct wl_surface* surface,
	struct wl_array* keys);

void willis_wayland_helpers_listener_keyboard_leave(
	void* data,
	struct wl_keyboard* keyboard,
	uint32_t serial,
	struct wl_surface* surface);

void willis_wayland_helpers_listener_keyboard_key(
	void* data,
	struct wl_keyboard* keyboard,
	uint32_t serial,
	uint32_t time,
	uint32_t key,
	uint32_t state);

void willis_wayland_helpers_listener_keyboard_modifiers(
	void* data,
	struct wl_keyboard* keyboard,
	uint32_t serial,
	uint32_t mods_depressed,
	uint32_t mods_latched,
	uint32_t mods_locked,
	uint32_t group);

void willis_wayland_helpers_listener_keyboard_repeat_info(
	void* data,
	struct wl_keyboard* keyboard,
	int32_t rate,
	int32_t delay);

void willis_wayland_helpers_listener_pointer_relative(
	void* data,
	struct zwp_relative_pointer_v1* pointer,
	uint32_t time_msp,
	uint32_t time_lsp,
	wl_fixed_t x,
	wl_fixed_t y,
	wl_fixed_t x_linear,
	wl_fixed_t y_linear);

void willis_wayland_helpers_listener_pointer_locked(
	void* data,
	struct zwp_locked_pointer_v1* locked);

void willis_wayland_helpers_listener_pointer_unlocked(
	void* data,
	struct zwp_locked_pointer_v1* locked);

#endif
