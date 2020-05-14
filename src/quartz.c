#include "willis.h"
#include "willis_events.h"

#include <objc/message.h>
#include <objc/runtime.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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

struct willis_size
{
	double width;
	double height;
};

struct willis_rect
{
	struct willis_point origin;
	struct willis_size size;
};

id (*willis_msg_id)(id, SEL) =
	(id (*)(id, SEL)) objc_msgSend;

void (*willis_msg_rect)(struct willis_rect*, id, SEL) =
	(void (*)(struct willis_rect*, id, SEL)) objc_msgSend_stret;

// for some reason the special objc_msgSend_stret returns a zeroed struct here
struct willis_point (*willis_msg_point)(id, SEL) =
	(struct willis_point (*)(id, SEL)) objc_msgSend;

// for some reason we have to give these a pointer to their caller instance
int (*willis_msg_int)(id*, SEL) =
	(int (*)(id*, SEL)) objc_msgSend;

double (*willis_msg_float)(id*, SEL) =
	(double (*)(id*, SEL)) objc_msgSend_fpret;

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
			// it seems that one event is sent for each mouse wheel step
			// so we can just check the sign to get the direction information

			// TODO check what happens on a tactile mouse
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
			// this event is an asynchronous movement notification: it does not
			// preserve the value of the mouse position at the time of emission
			// (mouseLocation always holds the current mouse coordinates)
			//
			// because of this we simply use the method from NSWindow instead
			// (since it returns local coordinates) instead of taking the
			// screen-space value available in the event
			id window = willis_msg_id(
				event,
				sel_getUid("window"));

			id view = willis_msg_id(
				window,
				sel_getUid("contentView"));

			// we can get this struct the regular way (see declaration)
			struct willis_rect rect = {0};

			willis_msg_rect(
				&rect,
				view,
				sel_getUid("frame"));

			// but the objective-c runtime breaks here so we do this shit
			struct willis_point point = willis_msg_point(
				window,
				sel_getUid("mouseLocationOutsideOfEventStream"));

			// because why the fuck would apple point that fucking y-axis
			// in the same fucking direction every other display system does
			// (we use stric operators to avoid apple trademarked bugs)
			if ((rect.size.height > point.y - rect.origin.y)
				&& (point.y > rect.origin.y)
				&& (rect.size.width > point.x - rect.origin.x)
				&& (point.x > rect.origin.x))
			{
				event_code = WILLIS_MOUSE_MOTION;
				event_state = WILLIS_STATE_NONE;

				willis->mouse_x = point.x - rect.origin.x;
				willis->mouse_y = rect.size.height - (point.y - rect.origin.y);
			}

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
