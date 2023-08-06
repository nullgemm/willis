#ifndef H_WILLIS_INTERNAL_WIN
#define H_WILLIS_INTERNAL_WIN

#include "willis.h"
#include "common/willis_error.h"

#include <stdint.h>
#include <stdbool.h>
#include <windows.h>

struct win_backend
{
	HWND win;
	HDC device_context;
	bool mouse_grabbed;
};

void willis_win_init(
	struct willis* context,
	struct willis_error_info* error);

void willis_win_start(
	struct willis* context,
	void* data,
	struct willis_error_info* error);

void willis_win_handle_event(
	struct willis* context,
	void* event,
	struct willis_event_info* event_info,
	struct willis_error_info* error);

bool willis_win_mouse_grab(
	struct willis* context,
	struct willis_error_info* error);

bool willis_win_mouse_ungrab(
	struct willis* context,
	struct willis_error_info* error);

void willis_win_stop(
	struct willis* context,
	struct willis_error_info* error);

void willis_win_clean(
	struct willis* context,
	struct willis_error_info* error);

#endif
