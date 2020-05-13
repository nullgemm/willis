#include "willis.h"
#include "willis_events.h"

#include <objc/message.h>
#include <objc/runtime.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum NSEventType
{
	NSEventTypeLeftMouseDown = 1,
	NSEventTypeLeftMouseUp = 2,
	NSEventTypeRightMouseDown = 3,
	NSEventTypeRightMouseUp = 4,
	NSEventTypeOtherMouseDown = 25,
	NSEventTypeOtherMouseUp = 26,
	NSEventTypeScrollWheel = 22,
	NSEventTypeMouseMoved = 5,
	NSEventTypeKeyDown = 10,
	NSEventTypeKeyUp = 11,
};

struct willis_point
{
	double x;
	double y;
};

struct willis_point (*willis_msg_point)(id*, SEL) =
	(struct willis_point (*)(id*, SEL)) objc_msgSend;

int (*willis_msg_int)(id*, SEL) =
	(int (*)(id*, SEL)) objc_msgSend;

double (*willis_msg_float)(id*, SEL) =
	(double (*)(id*, SEL)) objc_msgSend;

unsigned long (*willis_msg_type)(id*, SEL) =
	(unsigned long(*)(id*, SEL)) objc_msgSend;

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
	enum willis_event_code event_code = WILLIS_NONE;
	enum willis_event_state event_state;

	unsigned long type = willis_msg_type(event, sel_getUid("type"));

	switch (type)
	{
		case NSEventTypeLeftMouseDown:
		{
			event_code = WILLIS_MOUSE_CLICK_LEFT;
			event_state = WILLIS_STATE_PRESS;

			break;
		}
		case NSEventTypeLeftMouseUp:
		{
			event_code = WILLIS_MOUSE_CLICK_LEFT;
			event_state = WILLIS_STATE_RELEASE;

			break;
		}
		case NSEventTypeRightMouseDown:
		{
			event_code = WILLIS_MOUSE_CLICK_RIGHT;
			event_state = WILLIS_STATE_PRESS;

			break;
		}
		case NSEventTypeRightMouseUp:
		{
			event_code = WILLIS_MOUSE_CLICK_RIGHT;
			event_state = WILLIS_STATE_RELEASE;

			break;
		}
		case NSEventTypeOtherMouseDown:
		{
			// wtf apple
			int button = willis_msg_int(event, sel_getUid("buttonNumber"));

			if (button == 2)
			{
				event_code = WILLIS_MOUSE_CLICK_MIDDLE;
				event_state = WILLIS_STATE_PRESS;
			}

			break;
		}
		case NSEventTypeOtherMouseUp:
		{
			int button = willis_msg_int(event, sel_getUid("buttonNumber"));

			if (button == 2)
			{
				event_code = WILLIS_MOUSE_CLICK_MIDDLE;
				event_state = WILLIS_STATE_RELEASE;
			}

			break;
		}
		case NSEventTypeScrollWheel:
		{
			event_state = WILLIS_STATE_NONE;

			// thanks apple
			double steps =
				willis_msg_float(
					event,
					sel_getUid("scrollingDeltaY"));

			if (steps > 0.0)
			{
				event_code = WILLIS_MOUSE_WHEEL_DOWN;
			}
			else
			{
				event_code = WILLIS_MOUSE_WHEEL_UP;
			}

			break;
		}
		case NSEventTypeMouseMoved:
		{
			event_code = WILLIS_MOUSE_MOTION;
			event_state = WILLIS_STATE_NONE;

			struct willis_point point =
				willis_msg_point(
					event,
					sel_getUid("mouseLocation"));

			willis->mouse_x = point.x;
			willis->mouse_y = point.y;

			break;
		}
		case NSEventTypeKeyDown:
		{
			event_state = WILLIS_STATE_PRESS;

			break;
		}
		case NSEventTypeKeyUp:
		{
			event_state = WILLIS_STATE_RELEASE;

			break;
		}
	}

	if (event_code != WILLIS_NONE)
	{
		willis->callback(willis, event_code, event_state, willis->data);
	}
}

bool willis_free(struct willis* willis)
{
	return true;
}
