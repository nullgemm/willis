#include "include/willis.h"
#include "common/willis_private.h"
#include "include/willis_win.h"
#include "win/win.h"
#include "win/win_helpers.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winuser.h>

union willis_mixed_param
{
	uint64_t u;
	int64_t s;
};

void willis_win_init(
	struct willis* context,
	struct willis_error_info* error)
{
	// init win struct
	struct win_backend* backend = malloc(sizeof (struct win_backend));

	if (backend == NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_ALLOC);
		return;
	}

	struct win_backend zero = {0};
	*backend = zero;

	context->backend_data = backend;

	// success
	willis_error_ok(error);
}

void willis_win_start(
	struct willis* context,
	void* data,
	struct willis_error_info* error)
{
	struct win_backend* backend = context->backend_data;
	struct willis_win_data* window_data = data;

	backend->win = window_data->win;
	backend->device_context = window_data->device_context;
	backend->mouse_grabbed = false;

	willis_error_ok(error);
}

void willis_win_handle_event(
	struct willis* context,
	void* event,
	struct willis_event_info* event_info,
	struct willis_error_info* error)
{
	struct win_backend* backend = context->backend_data;

	// initialize event code and state to default values
	enum willis_event_code event_code = WILLIS_NONE;
	enum willis_event_state event_state = WILLIS_STATE_NONE;

	// initialize here to make the switch below more readable
	willis_error_ok(error);
	event_info->event_code = event_code;
	event_info->event_state = event_state;
	event_info->utf8_string = NULL;
	event_info->utf8_size = 0;
	event_info->mouse_wheel_steps = 0;
	event_info->mouse_x = 0;
	event_info->mouse_y = 0;
	event_info->diff_x = 0;
	event_info->diff_y = 0;

	// handle event
	MSG* msg = event;

	switch (msg->message)
	{
		case WM_KEYDOWN:
		{
			event_code = win_helpers_keycode_table(msg->wParam & 0xFF);
			event_state = WILLIS_STATE_PRESS;

			break;
		}
		case WM_KEYUP:
		{
			event_code = win_helpers_keycode_table(msg->wParam & 0xFF);
			event_state = WILLIS_STATE_RELEASE;

			break;
		}
		case WM_SYSKEYDOWN:
		{
			uint8_t code = msg->wParam & 0xFF;

			if (code == VK_MENU)
			{
				event_state = WILLIS_STATE_PRESS;

				if ((HIWORD(msg->lParam) & 0x0100) != 0)
				{
					event_code = WILLIS_KEY_ALT_RIGHT;
				}
				else
				{
					event_code = WILLIS_KEY_ALT_LEFT;
				}
			}
			else if (code == VK_F10)
			{
				event_state = WILLIS_STATE_PRESS;
				event_code = WILLIS_KEY_F10;
			}

			break;
		}
		case WM_SYSKEYUP:
		{
			uint8_t code = msg->wParam & 0xFF;

			if (code == VK_MENU)
			{
				event_state = WILLIS_STATE_RELEASE;

				if ((HIWORD(msg->lParam) & 0x0100) != 0)
				{
					event_code = WILLIS_KEY_ALT_RIGHT;
				}
				else
				{
					event_code = WILLIS_KEY_ALT_LEFT;
				}
			}
			else if (code == VK_F10)
			{
				event_state = WILLIS_STATE_RELEASE;
				event_code = WILLIS_KEY_F10;
			}

			break;
		}
		case WM_MOUSEWHEEL:
		{
			// mouse wheel steps portable sign reproduction
			uint8_t bit_length = (8 * (sizeof (WORD)));
			WORD sign = 1 << (bit_length - 1);
			WORD param = HIWORD(msg->wParam);

			union willis_mixed_param mixed_param;
			mixed_param.u = param;

			if ((param & sign) != sign)
			{
				event_code = WILLIS_MOUSE_WHEEL_UP;
			}
			else
			{
				mixed_param.u |= 0xFFFFFFFFFFFFFFFF << bit_length;
				mixed_param.s = -mixed_param.s;
				event_code = WILLIS_MOUSE_WHEEL_DOWN;
			}

			event_info->mouse_wheel_steps = mixed_param.u / WHEEL_DELTA;

			break;
		}
		case WM_LBUTTONDOWN:
		{
			event_code = WILLIS_MOUSE_CLICK_LEFT;
			event_state = WILLIS_STATE_PRESS;

			break;
		}
		case WM_LBUTTONUP:
		{
			event_code = WILLIS_MOUSE_CLICK_LEFT;
			event_state = WILLIS_STATE_RELEASE;

			break;
		}
		case WM_RBUTTONDOWN:
		{
			event_code = WILLIS_MOUSE_CLICK_RIGHT;
			event_state = WILLIS_STATE_PRESS;

			break;
		}
		case WM_RBUTTONUP:
		{
			event_code = WILLIS_MOUSE_CLICK_RIGHT;
			event_state = WILLIS_STATE_RELEASE;

			break;
		}
		case WM_MBUTTONDOWN:
		{
			event_code = WILLIS_MOUSE_CLICK_MIDDLE;
			event_state = WILLIS_STATE_PRESS;

			break;
		}
		case WM_MBUTTONUP:
		{
			event_code = WILLIS_MOUSE_CLICK_MIDDLE;
			event_state = WILLIS_STATE_RELEASE;

			break;
		}
		case WM_MOUSEMOVE:
		{
			event_code = WILLIS_MOUSE_MOTION;
			event_state = WILLIS_STATE_NONE;

			event_info->mouse_x = LOWORD(msg->lParam);
			event_info->mouse_y = HIWORD(msg->lParam);

			break;
		}
		case WM_INPUT:
		{
			RAWINPUT raw = {0};
			UINT raw_bytes = sizeof (RAWINPUT);
			HRAWINPUT input = (HRAWINPUT) msg->lParam;

			UINT ok =
				GetRawInputData(
					input,
					RID_INPUT,
					&raw,
					&raw_bytes,
					sizeof (RAWINPUTHEADER));

			if (ok == ((UINT)-1))
			{
				willis_error_throw(
					context,
					error,
					WILLIS_ERROR_WIN_WINDOW_MOUSE_RAW_GET);

				break;
			}

			if (raw.header.dwType == RIM_TYPEMOUSE)
			{
				RAWMOUSE* mouse = &raw.data.mouse;

				if ((mouse->usFlags & 0x01) == MOUSE_MOVE_RELATIVE)
				{
					event_code = WILLIS_MOUSE_MOTION;
					event_state = WILLIS_STATE_NONE;

					event_info->diff_x = mouse->lLastX * 0x0000000100000000;
					event_info->diff_y = mouse->lLastY * 0x0000000100000000;
				}
			}

			break;
		}
		case WM_CHAR:
		{
			// utf16 to utf8 conversion
			uint32_t utf16 = msg->wParam;
			event_info->utf8_string = malloc(5);

			if (event_info->utf8_string == NULL)
			{
				willis_error_throw(context, error, WILLIS_ERROR_ALLOC);
				break;
			}

			if (utf16 < 0x80)
			{
				event_info->utf8_string[0] = utf16;
				event_info->utf8_size = 1;
			}
			else if (utf16 < 0x800)
			{
				event_info->utf8_string[0] = 0xC0 | (0x1F & (utf16 >> 6));
				event_info->utf8_string[1] = 0x80 | (0x3F & (utf16 >> 0));
				event_info->utf8_size = 2;
			}
			else if (utf16 < 0x10000)
			{
				event_info->utf8_string[0] = 0xE0 | (0x0F & (utf16 >> 12));
				event_info->utf8_string[1] = 0x80 | (0x3F & (utf16 >> 6));
				event_info->utf8_string[2] = 0x80 | (0x3F & (utf16 >> 0));
				event_info->utf8_size = 3;
			}
			else
			{
				event_info->utf8_string[0] = 0xF0 | (0x07 & (utf16 >> 18));
				event_info->utf8_string[1] = 0x80 | (0x3F & (utf16 >> 12));
				event_info->utf8_string[2] = 0x80 | (0x3F & (utf16 >> 6));
				event_info->utf8_string[3] = 0x80 | (0x3F & (utf16 >> 0));
				event_info->utf8_size = 4;
			}

			event_info->utf8_string[event_info->utf8_size] = '\0';

			break;
		}
		default:
		{
			break;
		}
	}

	event_info->event_code = event_code;
	event_info->event_state = event_state;

	// error always set
}

// this might look like a dirty way to work around the device listing system
// but these values are actually documented as valid for any mouse in the Winuser.h manual
// (https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rid_device_info_mouse)
bool willis_win_mouse_grab(
	struct willis* context,
	struct willis_error_info* error)
{
	struct win_backend* backend = context->backend_data;

	// abort if already grabbed
	if (backend->mouse_grabbed == true)
	{
		willis_error_ok(error);
		return false;
	}

	BOOL ok;

	// register raw mouse input access
	RAWINPUTDEVICE mouse = {1, 2, 0, backend->win};

	ok = RegisterRawInputDevices(&mouse, 1, sizeof (RAWINPUTDEVICE));

	if (ok == FALSE)
	{
		willis_error_throw(context, error, WILLIS_ERROR_WIN_MOUSE_GRAB);
		return false;
	}

	// get window rect
	RECT rect;

	ok = GetWindowRect(backend->win, &rect);

	if (ok == 0)
	{
		willis_error_throw(context, error, WILLIS_ERROR_WIN_WINDOW_RECT_GET);
		return false;
	}

	// clip cursor
	LONG x = (rect.left + rect.right) / 2;
	LONG y = (rect.top + rect.bottom) / 2;

	rect.left = x - 1;
	rect.right = x + 1;
	rect.top = y - 1;
	rect.bottom = y + 1;

	ok = ClipCursor(&rect);

	if (ok == 0)
	{
		willis_error_throw(context, error, WILLIS_ERROR_WIN_WINDOW_CURSOR_CLIP);
		return false;
	}

	// save grab status
	ShowCursor(FALSE);
	backend->mouse_grabbed = true;

	// error always set
	return true;
}

bool willis_win_mouse_ungrab(
	struct willis* context,
	struct willis_error_info* error)
{
	struct win_backend* backend = context->backend_data;

	// abort if already ungrabbed
	if (backend->mouse_grabbed == false)
	{
		willis_error_ok(error);
		return false;
	}

	BOOL ok;

	// unregister raw mouse input access
	RAWINPUTDEVICE mouse = {1, 2, RIDEV_REMOVE, NULL};

	ok = RegisterRawInputDevices(&mouse, 1, sizeof (RAWINPUTDEVICE));

	if (ok == FALSE)
	{
		willis_error_throw(context, error, WILLIS_ERROR_WIN_MOUSE_UNGRAB);
		return false;
	}

	// unclip cursor
	ok = ClipCursor(NULL);

	if (ok == 0)
	{
		willis_error_throw(context, error, WILLIS_ERROR_WIN_WINDOW_CURSOR_UNCLIP);
		return false;
	}

	// save grab status
	ShowCursor(TRUE);
	backend->mouse_grabbed = false;

	// error always set
	return true;
}

void willis_win_stop(
	struct willis* context,
	struct willis_error_info* error)
{
	struct win_backend* backend = context->backend_data;

	willis_error_ok(error);
}

void willis_win_clean(
	struct willis* context,
	struct willis_error_info* error)
{
	struct win_backend* backend = context->backend_data;

	free(backend);

	willis_error_ok(error);
}

void willis_prepare_init_win(
	struct willis_config_backend* config)
{
	config->data = NULL;
	config->init = willis_win_init;
	config->start = willis_win_start;
	config->handle_event = willis_win_handle_event;
	config->mouse_grab = willis_win_mouse_grab;
	config->mouse_ungrab = willis_win_mouse_ungrab;
	config->stop = willis_win_stop;
	config->clean = willis_win_clean;
}
