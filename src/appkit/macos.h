#ifndef H_WILLIS_INTERNAL_APPKIT
#define H_WILLIS_INTERNAL_APPKIT

#include "willis.h"
#include "common/willis_error.h"

#include <stdint.h>
#include <stdbool.h>

struct appkit_backend
{
	int old_flags;
	bool mouse_grabbed;
	bool capslock_enabled;
};

void willis_appkit_init(
	struct willis* context,
	struct willis_error_info* error);

void willis_appkit_start(
	struct willis* context,
	void* data,
	struct willis_error_info* error);

void willis_appkit_handle_event(
	struct willis* context,
	void* event,
	struct willis_event_info* event_info,
	struct willis_error_info* error);

bool willis_appkit_mouse_grab(
	struct willis* context,
	struct willis_error_info* error);

bool willis_appkit_mouse_ungrab(
	struct willis* context,
	struct willis_error_info* error);

void willis_appkit_stop(
	struct willis* context,
	struct willis_error_info* error);

void willis_appkit_clean(
	struct willis* context,
	struct willis_error_info* error);

#endif
