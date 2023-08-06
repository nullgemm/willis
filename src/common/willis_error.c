#include "include/willis.h"
#include "common/willis_private.h"
#include "common/willis_error.h"

#include <stdbool.h>

#if defined(WILLIS_ERROR_LOG_MANUAL) || defined(WILLIS_ERROR_LOG_THROW)
	#include <stdio.h>
#endif

#ifdef WILLIS_ERROR_ABORT
	#include <stdlib.h>
#endif

void willis_error_init(
	struct willis* context)
{
#ifndef WILLIS_ERROR_SKIP
	char** log = context->error_messages;

	log[WILLIS_ERROR_OK] =
		"out-of-bound error message";
	log[WILLIS_ERROR_NULL] =
		"null pointer";
	log[WILLIS_ERROR_ALLOC] =
		"failed malloc";
	log[WILLIS_ERROR_BOUNDS] =
		"out-of-bounds index";
	log[WILLIS_ERROR_DOMAIN] =
		"invalid domain";
	log[WILLIS_ERROR_FD] =
		"invalid file descriptor";

	log[WILLIS_ERROR_EVENT_CODE_INVALID] =
		"invalid event code";
	log[WILLIS_ERROR_EVENT_STATE_INVALID] =
		"invalid event state";

	log[WILLIS_ERROR_X11_XFIXES_VERSION] =
		"couldn't get required Xfixes version";
	log[WILLIS_ERROR_X11_XFIXES_HIDE] =
		"couldn't hide the cursor with Xfixes";
	log[WILLIS_ERROR_X11_XFIXES_SHOW] =
		"couldn't show the cursor with Xfixes";
	log[WILLIS_ERROR_X11_GRAB] =
		"couldn't grab the mouse pointer";
	log[WILLIS_ERROR_X11_UNGRAB] =
		"couldn't ungrab the mouse pointer";
	log[WILLIS_ERROR_X11_XINPUT_SELECT_EVENTS] =
		"couldn't select events with Xinput";
	log[WILLIS_ERROR_X11_XINPUT_GET_POINTER] =
		"couldn't get pointer with Xinput";
	log[WILLIS_ERROR_X11_XKB_SETUP] =
		"couldn't initialize the XKB X11 extension";
	log[WILLIS_ERROR_X11_XKB_DEVICE_GET] =
		"couldn't get the device id with XKB";
	log[WILLIS_ERROR_X11_XKB_KEYMAP_NEW] =
		"couldn't create a new XKB keymap";
	log[WILLIS_ERROR_X11_XKB_STATE_NEW] =
		"couldn't create a new XKB state";
	log[WILLIS_ERROR_X11_XKB_SELECT_EVENTS] =
		"couldn't select events with XKB";
	log[WILLIS_ERROR_XKB_CONTEXT_NEW] =
		"couldn't create a new XKB context";

	log[WILLIS_ERROR_WIN_MOUSE_GRAB] =
		"couldn't grab win32 mouse";
	log[WILLIS_ERROR_WIN_MOUSE_UNGRAB] =
		"couldn't ungrab win32 mouse";
	log[WILLIS_ERROR_WIN_WINDOW_RECT_GET] =
		"couldn't get win32 window rectangle";
	log[WILLIS_ERROR_WIN_WINDOW_CURSOR_CLIP] =
		"couldn't clip win32 mouse";
	log[WILLIS_ERROR_WIN_WINDOW_CURSOR_UNCLIP] =
		"couldn't unclip win32 mouse";
	log[WILLIS_ERROR_WIN_WINDOW_MOUSE_RAW_GET] =
		"couldn't get win32 window raw mouse movement info";
#endif
}

void willis_error_log(
	struct willis* context,
	struct willis_error_info* error)
{
#ifndef WILLIS_ERROR_SKIP
	#ifdef WILLIS_ERROR_LOG_MANUAL
		#ifdef WILLIS_ERROR_LOG_DEBUG
			fprintf(
				stderr,
				"error in %s line %u: ",
				error->file,
				error->line);
		#endif

		if (error->code < WILLIS_ERROR_COUNT)
		{
			fprintf(stderr, "%s\n", context->error_messages[error->code]);
		}
		else
		{
			fprintf(stderr, "%s\n", context->error_messages[0]);
		}
	#endif
#endif
}

const char* willis_error_get_msg(
	struct willis* context,
	struct willis_error_info* error)
{
	if (error->code < WILLIS_ERROR_COUNT)
	{
		return context->error_messages[error->code];
	}
	else
	{
		return context->error_messages[0];
	}
}

enum willis_error willis_error_get_code(
	struct willis_error_info* error)
{
	return error->code;
}

const char* willis_error_get_file(
	struct willis_error_info* error)
{
	return error->file;
}

unsigned willis_error_get_line(
	struct willis_error_info* error)
{
	return error->line;
}

void willis_error_ok(
	struct willis_error_info* error)
{
	error->code = WILLIS_ERROR_OK;
	error->file = "";
	error->line = 0;
}

void willis_error_throw_extra(
	struct willis* context,
	struct willis_error_info* error,
	enum willis_error code,
	const char* file,
	unsigned line)
{
#ifndef WILLIS_ERROR_SKIP
	error->code = code;
	error->file = file;
	error->line = line;

	#ifdef WILLIS_ERROR_LOG_THROW
		#ifdef WILLIS_ERROR_LOG_DEBUG
			fprintf(
				stderr,
				"error in %s line %u: ",
				file,
				line);
		#endif

		if (error->code < WILLIS_ERROR_COUNT)
		{
			fprintf(stderr, "%s\n", context->error_messages[error->code]);
		}
		else
		{
			fprintf(stderr, "%s\n", context->error_messages[0]);
		}
	#endif

	#ifdef WILLIS_ERROR_ABORT
		abort();
	#endif
#endif
}
