#include "willis.h"
#include "willis_events.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>
#include <stdio.h>

union mixed_param
{
	uint64_t u;
	int64_t s;
};

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
	willis->callback = callback;
	willis->data = data;
	willis->utf8_string = NULL;
	willis->utf8_size = 0;
	willis->get_utf8 = utf8;

	return true;
}

void willis_handle_events(
	void* event,
	void* ctx)
{
	struct willis* willis = ctx;
	MSG* msg = event;

	enum willis_event_code event_code = WILLIS_NONE;
	enum willis_event_state event_state;

	switch (msg->message)
	{
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
		case WM_MOUSEWHEEL:
		{
			uint8_t bit_length = (8 * (sizeof (WORD)));
			WORD sign = 1 << (bit_length - 1);
			WORD param = HIWORD(msg->wParam);

			union mixed_param mixed_param;
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

			mixed_param.u /= WHEEL_DELTA;

			for (uint32_t i = 0; i < mixed_param.u; ++i)
			{
				willis->callback(
					willis,
					event_code,
					WILLIS_STATE_NONE,
					willis->data);
			}

			return;
		}
		default:
		{
			break;
		}
	}

	if (event_code != WILLIS_NONE)
	{
		willis->callback(willis, event_code, event_state, willis->data);

		if (willis->utf8_string != NULL)
		{
			free(willis->utf8_string);
			willis->utf8_string = NULL;
			willis->utf8_size = 0;
		}
	}
}

bool willis_free(struct willis* willis)
{
	return true;
}
