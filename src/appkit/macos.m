#include "include/willis.h"
#include "common/willis_private.h"
#include "include/willis_appkit.h"
#include "appkit/macos.h"
#include "appkit/macos_helpers.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#import <Carbon/Carbon.h> // virtual keycode enums
#import <CoreGraphics/CoreGraphics.h> // CGDisplayHideCursor, CGDisplayShowCursor

void willis_appkit_init(
	struct willis* context,
	struct willis_error_info* error)
{
	// init appkit struct
	struct appkit_backend* backend = malloc(sizeof (struct appkit_backend));

	if (backend == NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_ALLOC);
		return;
	}

	struct appkit_backend zero = {0};
	*backend = zero;

	context->backend_data = backend;

	// success
	willis_error_ok(error);
}

void willis_appkit_start(
	struct willis* context,
	void* data,
	struct willis_error_info* error)
{
	struct appkit_backend* backend = context->backend_data;
	struct willis_appkit_data* window_data = data;

	backend->mouse_grabbed = false;

	willis_error_ok(error);
}

void willis_appkit_handle_event(
	struct willis* context,
	void* event,
	struct willis_event_info* event_info,
	struct willis_error_info* error)
{
	struct appkit_backend* backend = context->backend_data;

	// initialize event code and state to default values
	enum willis_event_code event_code = WILLIS_NONE;
	enum willis_event_state event_state = WILLIS_STATE_NONE;

	willis_error_ok(error);
	event_info->event_code = event_code;
	event_info->event_state = event_state;
	event_info->utf8_string = NULL;
	event_info->utf8_size = 0;
	event_info->mouse_x = 0;
	event_info->mouse_y = 0;
	event_info->diff_x = 0;
	event_info->diff_y = 0;

	// handle event
	NSEvent* nsevent = (NSEvent*) event;
	NSEventType type = [nsevent type];

	switch (type)
	{
		case NSEventTypeLeftMouseDown:
		{
			event_info->event_code = WILLIS_MOUSE_CLICK_LEFT;
			event_info->event_state = WILLIS_STATE_PRESS;
			break;
		}
		case NSEventTypeLeftMouseUp:
		{
			event_info->event_code = WILLIS_MOUSE_CLICK_LEFT;
			event_info->event_state = WILLIS_STATE_RELEASE;
			break;
		}
		case NSEventTypeRightMouseDown:
		{
			event_info->event_code = WILLIS_MOUSE_CLICK_RIGHT;
			event_info->event_state = WILLIS_STATE_PRESS;
			break;
		}
		case NSEventTypeRightMouseUp:
		{
			event_info->event_code = WILLIS_MOUSE_CLICK_RIGHT;
			event_info->event_state = WILLIS_STATE_RELEASE;
			break;
		}
		case NSEventTypeOtherMouseDown:
		{
			if ([nsevent buttonNumber] == 2)
			{
				event_info->event_code = WILLIS_MOUSE_CLICK_MIDDLE;
				event_info->event_state = WILLIS_STATE_PRESS;
			}

			break;
		}
		case NSEventTypeOtherMouseUp:
		{
			if ([nsevent buttonNumber] == 2)
			{
				event_info->event_code = WILLIS_MOUSE_CLICK_MIDDLE;
				event_info->event_state = WILLIS_STATE_RELEASE;
			}

			break;
		}
		case NSEventTypeLeftMouseDragged:
		case NSEventTypeRightMouseDragged:
		case NSEventTypeOtherMouseDragged:
		case NSEventTypeMouseMoved:
		{
			if (backend->mouse_grabbed == false)
			{
				// this event is an asynchronous movement notification: it does not
				// preserve the value of the mouse position at the time of emission
				// (mouseLocation always holds the current mouse coordinates)
				//
				// because of this we simply use the method from NSWindow instead
				// (since it returns local coordinates) instead of taking the
				// screen-space value available in NSEvent

				id window = [nsevent window];
				id view = [window contentView];
				NSRect frame = [view frame];
				NSPoint point = [window mouseLocationOutsideOfEventStream];

				if ((NSHeight(frame) > (point.x - NSMinY(frame)))
				&& (point.y > NSMinY(frame))
				&& (NSWidth(frame) > (point.x - NSMinX(frame)))
				&& (point.x > NSMinX(frame)))
				{
					event_info->event_code = WILLIS_MOUSE_MOTION;
					event_info->event_state = WILLIS_STATE_NONE;
					event_info->mouse_x = point.x - NSMinX(frame);
					event_info->mouse_y = NSHeight(frame) - (point.y - NSMinY(frame));
				}
			}
			else
			{
				double diff_x = [nsevent deltaX];
				double diff_y = [nsevent deltaY];

				diff_x *= 0x0000000100000000;
				diff_y *= 0x0000000100000000;

				if (diff_x >= 0)
				{
					diff_x += 0.5;
				}
				else
				{
					diff_x -= 0.5;
				}

				if (diff_y >= 0)
				{
					diff_y += 0.5;
				}
				else
				{
					diff_y -= 0.5;
				}

				event_info->event_code = WILLIS_MOUSE_MOTION;
				event_info->event_state = WILLIS_STATE_NONE;
				event_info->diff_x = diff_x;
				event_info->diff_y = diff_y;
			}

			break;
		}
		case NSEventTypeKeyDown:
		{
			bool repeat = [nsevent isARepeat];

			if (repeat == false)
			{
				uint8_t code = [nsevent keyCode];
				event_info->event_code = appkit_helpers_keycode_table(code);
				event_info->event_state = WILLIS_STATE_PRESS;
			}

			id string = [nsevent characters];
			const char* str = [string UTF8String];

			event_info->utf8_string = strdup(str);
			event_info->utf8_size = strlen(str);

			break;
		}
		case NSEventTypeKeyUp:
		{
			bool repeat = [nsevent isARepeat];

			if (repeat == false)
			{
				uint8_t code = [nsevent keyCode];
				event_info->event_code = appkit_helpers_keycode_table(code);
				event_info->event_state = WILLIS_STATE_RELEASE;
			}

			id string = [nsevent characters];
			const char* str = [string UTF8String];

			event_info->utf8_string = strdup(str);
			event_info->utf8_size = strlen(str);

			break;
		}
		case NSEventTypeScrollWheel:
		{
			event_info->event_state = WILLIS_STATE_NONE;

			if ([nsevent scrollingDeltaY] > 0.0)
			{
				event_info->event_code = WILLIS_MOUSE_WHEEL_DOWN;
			}
			else
			{
				event_info->event_code = WILLIS_MOUSE_WHEEL_UP;
			}

			break;
		}
		case NSEventTypeFlagsChanged:
		{
			NSEventModifierFlags flags = [nsevent modifierFlags];
			uint8_t code = [nsevent keyCode];

			NSEventModifierFlags capslock_flags =
				flags & NSEventModifierFlagCapsLock;
			NSEventModifierFlags capslock_old_flags =
				backend->old_flags & NSEventModifierFlagCapsLock;

			if (capslock_flags != capslock_old_flags)
			{
				event_info->event_code = appkit_helpers_keycode_table(code);
				event_info->event_state = WILLIS_STATE_PRESS;
				backend->capslock_enabled = true;
			}

			appkit_helpers_send_system_keys(
				backend,
				event_info,
				flags,
				NSEventModifierFlagShift,
				code);

			appkit_helpers_send_system_keys(
				backend,
				event_info,
				flags,
				NSEventModifierFlagControl,
				code);

			appkit_helpers_send_system_keys(
				backend,
				event_info,
				flags,
				NSEventModifierFlagOption,
				code);

			appkit_helpers_send_system_keys(
				backend,
				event_info,
				flags,
				NSEventModifierFlagCommand,
				code);

			backend->old_flags = flags;

			break;
		}
		case NSEventTypeSystemDefined:
		{
			if (backend->capslock_enabled == true)
			{
				event_info->event_code = appkit_helpers_keycode_table(kVK_CapsLock);
				event_info->event_state = WILLIS_STATE_RELEASE;
				backend->capslock_enabled = false;
			}

			break;
		}
		default:
		{
			break;
		}
	}

	// error always set
}

bool willis_appkit_mouse_grab(
	struct willis* context,
	struct willis_error_info* error)
{
	struct appkit_backend* backend = context->backend_data;
	willis_error_ok(error);

	// abort if already grabbed
	if (backend->mouse_grabbed == true)
	{
		return false;
	}

	CGAssociateMouseAndMouseCursorPosition(false);
	CGDisplayHideCursor(kCGDirectMainDisplay);
	backend->mouse_grabbed = true;

	// error always set
	return true;
}

bool willis_appkit_mouse_ungrab(
	struct willis* context,
	struct willis_error_info* error)
{
	struct appkit_backend* backend = context->backend_data;
	willis_error_ok(error);

	// abort if already ungrabbed
	if (backend->mouse_grabbed == false)
	{
		return false;
	}

	CGDisplayShowCursor(kCGDirectMainDisplay);
	CGAssociateMouseAndMouseCursorPosition(true);
	backend->mouse_grabbed = false;

	// error always set
	return true;
}

void willis_appkit_stop(
	struct willis* context,
	struct willis_error_info* error)
{
	struct appkit_backend* backend = context->backend_data;

	willis_error_ok(error);
}

void willis_appkit_clean(
	struct willis* context,
	struct willis_error_info* error)
{
	struct appkit_backend* backend = context->backend_data;

	free(backend);

	willis_error_ok(error);
}

void willis_prepare_init_appkit(
	struct willis_config_backend* config)
{
	config->data = NULL;
	config->init = willis_appkit_init;
	config->start = willis_appkit_start;
	config->handle_event = willis_appkit_handle_event;
	config->mouse_grab = willis_appkit_mouse_grab;
	config->mouse_ungrab = willis_appkit_mouse_ungrab;
	config->stop = willis_appkit_stop;
	config->clean = willis_appkit_clean;
}
