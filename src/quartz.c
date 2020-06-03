#include "willis.h"
#include "willis_events.h"

#include <objc/message.h>
#include <objc/runtime.h>

// virtual keycode enums
#include <Carbon/Carbon.h>
// CGDisplayHideCursor, CGDisplayShowCursor
#include <CoreGraphics/CoreGraphics.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum NSEventType
{
	NSEventTypeLeftMouseDown = 1,
	NSEventTypeLeftMouseUp = 2,
	NSEventTypeRightMouseDown = 3,
	NSEventTypeRightMouseUp = 4,
	NSEventTypeLeftMouseDragged = 6,
	NSEventTypeRightMouseDragged = 7,
	NSEventTypeOtherMouseDown = 25,
	NSEventTypeOtherMouseUp = 26,
	NSEventTypeOtherMouseDragged = 27,
	NSEventTypeScrollWheel = 22,
	NSEventTypeMouseMoved = 5,
	NSEventTypeKeyDown = 10,
	NSEventTypeKeyUp = 11,
	NSEventTypeFlagsChanged = 12,
	NSEventTypeSystemDefined = 14,
};

enum NSEventModifierFlags
{
	NSEventModifierFlagCapsLock = 1 << 16,
	NSEventModifierFlagShift = 1 << 17,
	NSEventModifierFlagControl = 1 << 18,
	NSEventModifierFlagOption = 1 << 19,
	NSEventModifierFlagCommand = 1 << 20,
};

static enum willis_event_code apple_keycode_table[256] =
{
[kVK_ANSI_A             ] = WILLIS_KEY_A,
[kVK_ANSI_S             ] = WILLIS_KEY_S,
[kVK_ANSI_D             ] = WILLIS_KEY_D,
[kVK_ANSI_F             ] = WILLIS_KEY_F,
[kVK_ANSI_H             ] = WILLIS_KEY_H,
[kVK_ANSI_G             ] = WILLIS_KEY_G,
[kVK_ANSI_Z             ] = WILLIS_KEY_Z,
[kVK_ANSI_X             ] = WILLIS_KEY_X,
[kVK_ANSI_C             ] = WILLIS_KEY_C,
[kVK_ANSI_V             ] = WILLIS_KEY_V,
[kVK_ANSI_B             ] = WILLIS_KEY_B,
[kVK_ANSI_Q             ] = WILLIS_KEY_Q,
[kVK_ANSI_W             ] = WILLIS_KEY_W,
[kVK_ANSI_E             ] = WILLIS_KEY_E,
[kVK_ANSI_R             ] = WILLIS_KEY_R,
[kVK_ANSI_Y             ] = WILLIS_KEY_Y,
[kVK_ANSI_T             ] = WILLIS_KEY_T,
[kVK_ANSI_1             ] = WILLIS_KEY_1,
[kVK_ANSI_2             ] = WILLIS_KEY_2,
[kVK_ANSI_3             ] = WILLIS_KEY_3,
[kVK_ANSI_4             ] = WILLIS_KEY_4,
[kVK_ANSI_6             ] = WILLIS_KEY_6,
[kVK_ANSI_5             ] = WILLIS_KEY_5,
[kVK_ANSI_Equal         ] = WILLIS_KEY_EQUALS,
[kVK_ANSI_9             ] = WILLIS_KEY_9,
[kVK_ANSI_7             ] = WILLIS_KEY_7,
[kVK_ANSI_Minus         ] = WILLIS_KEY_HYPHEN_MINUS,
[kVK_ANSI_8             ] = WILLIS_KEY_8,
[kVK_ANSI_0             ] = WILLIS_KEY_0,
[kVK_ANSI_RightBracket  ] = WILLIS_KEY_BRACKET_RIGHT,
[kVK_ANSI_O             ] = WILLIS_KEY_O,
[kVK_ANSI_U             ] = WILLIS_KEY_U,
[kVK_ANSI_LeftBracket   ] = WILLIS_KEY_BRACKET_LEFT,
[kVK_ANSI_I             ] = WILLIS_KEY_I,
[kVK_ANSI_P             ] = WILLIS_KEY_P,
[kVK_ANSI_L             ] = WILLIS_KEY_L,
[kVK_ANSI_J             ] = WILLIS_KEY_J,
[kVK_ANSI_Quote         ] = WILLIS_KEY_APOSTROPHE,
[kVK_ANSI_K             ] = WILLIS_KEY_K,
[kVK_ANSI_Semicolon     ] = WILLIS_KEY_SEMICOLON,
[kVK_ANSI_Backslash     ] = WILLIS_KEY_ANTISLASH,
[kVK_ANSI_Comma         ] = WILLIS_KEY_COMMA,
[kVK_ANSI_Slash         ] = WILLIS_KEY_SLASH,
[kVK_ANSI_N             ] = WILLIS_KEY_N,
[kVK_ANSI_M             ] = WILLIS_KEY_M,
[kVK_ANSI_Period        ] = WILLIS_KEY_PERIOD,
[kVK_ANSI_Grave         ] = WILLIS_KEY_GRAVE,
[kVK_ANSI_KeypadDecimal ] = WILLIS_KEY_NUM_DELETE,
[kVK_ANSI_KeypadMultiply] = WILLIS_KEY_NUM_ASTERISK,
[kVK_ANSI_KeypadPlus    ] = WILLIS_KEY_NUM_PLUS,
[kVK_ANSI_KeypadClear   ] = WILLIS_KEY_NUM_LOCK,
[kVK_ANSI_KeypadDivide  ] = WILLIS_KEY_NUM_SLASH,
[kVK_ANSI_KeypadEnter   ] = WILLIS_KEY_NUM_ENTER,
[kVK_ANSI_KeypadMinus   ] = WILLIS_KEY_NUM_HYPHEN_MINUS,
[kVK_ANSI_Keypad0       ] = WILLIS_KEY_NUM_0,
[kVK_ANSI_Keypad1       ] = WILLIS_KEY_NUM_1,
[kVK_ANSI_Keypad2       ] = WILLIS_KEY_NUM_2,
[kVK_ANSI_Keypad3       ] = WILLIS_KEY_NUM_3,
[kVK_ANSI_Keypad4       ] = WILLIS_KEY_NUM_4,
[kVK_ANSI_Keypad5       ] = WILLIS_KEY_NUM_5,
[kVK_ANSI_Keypad6       ] = WILLIS_KEY_NUM_6,
[kVK_ANSI_Keypad7       ] = WILLIS_KEY_NUM_7,
[kVK_ANSI_Keypad8       ] = WILLIS_KEY_NUM_8,
[kVK_ANSI_Keypad9       ] = WILLIS_KEY_NUM_9,
[kVK_Return             ] = WILLIS_KEY_ENTER,
[kVK_Tab                ] = WILLIS_KEY_TAB,
[kVK_Space              ] = WILLIS_KEY_SPACE,
[kVK_Delete             ] = WILLIS_KEY_BACKSPACE,
[kVK_Escape             ] = WILLIS_KEY_ESCAPE,
[kVK_Command            ] = WILLIS_KEY_MOD_LEFT,
[kVK_Shift              ] = WILLIS_KEY_SHIFT_LEFT,
[kVK_CapsLock           ] = WILLIS_KEY_CAPS_LOCK,
[kVK_Option             ] = WILLIS_KEY_ALT_LEFT,
[kVK_Control            ] = WILLIS_KEY_CTRL_LEFT,
[kVK_RightCommand       ] = WILLIS_KEY_MOD_RIGHT,
[kVK_RightShift         ] = WILLIS_KEY_SHIFT_RIGHT,
[kVK_RightOption        ] = WILLIS_KEY_ALT_RIGHT,
[kVK_RightControl       ] = WILLIS_KEY_CTRL_RIGHT,
[kVK_F5                 ] = WILLIS_KEY_F5,
[kVK_F6                 ] = WILLIS_KEY_F6,
[kVK_F7                 ] = WILLIS_KEY_F7,
[kVK_F3                 ] = WILLIS_KEY_F3,
[kVK_F8                 ] = WILLIS_KEY_F8,
[kVK_F9                 ] = WILLIS_KEY_F9,
[kVK_F11                ] = WILLIS_KEY_F11,
[kVK_F10                ] = WILLIS_KEY_F10,
[0x6E                   ] = WILLIS_KEY_MENU,
[kVK_F12                ] = WILLIS_KEY_F12,
[kVK_F13                ] = WILLIS_KEY_PRINT_SCREEN,
[kVK_Help               ] = WILLIS_KEY_INSERT,
[kVK_Home               ] = WILLIS_KEY_HOME,
[kVK_PageUp             ] = WILLIS_KEY_PAGE_UP,
[kVK_ForwardDelete      ] = WILLIS_KEY_DELETE,
[kVK_F4                 ] = WILLIS_KEY_F4,
[kVK_End                ] = WILLIS_KEY_END,
[kVK_F2                 ] = WILLIS_KEY_F2,
[kVK_PageDown           ] = WILLIS_KEY_PAGE_DOWN,
[kVK_F1                 ] = WILLIS_KEY_F1,
[kVK_LeftArrow          ] = WILLIS_KEY_LEFT,
[kVK_RightArrow         ] = WILLIS_KEY_RIGHT,
[kVK_DownArrow          ] = WILLIS_KEY_DOWN,
[kVK_UpArrow            ] = WILLIS_KEY_UP,
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

const char* (*willis_msg_utf8)(id, SEL) =
	(const char* (*)(id, SEL)) objc_msgSend;

uint16_t (*willis_msg_u16)(id, SEL) =
	(uint16_t (*)(id, SEL)) objc_msgSend;

bool (*willis_msg_bool)(id, SEL) =
	(bool (*)(id, SEL)) objc_msgSend;

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

static inline void send_system_keys(
	struct willis* willis,
	enum NSEventModifierFlags flags,
	enum NSEventModifierFlags comparison,
	uint8_t code)
{
	enum NSEventModifierFlags lone = flags & comparison;
	enum willis_event_state event_state;
	enum willis_event_code event_code;

	if (lone != (willis->quartz_old_flags & comparison))
	{
		event_code = apple_keycode_table[code];

		if (lone != 0)
		{
			event_state = WILLIS_STATE_PRESS;
		}
		else
		{
			event_state = WILLIS_STATE_RELEASE;
		}

		willis->callback(willis, event_code, event_state, willis->data);
	}
}

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
	willis->quartz_old_flags = 0;
	willis->quartz_capslock = false;
	willis->mouse_grab = false;
	willis->mouse_x = 0;
	willis->mouse_y = 0;
	willis->diff_x = 0;
	willis->diff_y = 0;

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
		case NSEventTypeLeftMouseDragged:
		case NSEventTypeRightMouseDragged:
		case NSEventTypeOtherMouseDragged:
		case NSEventTypeMouseMoved:
		{
			if (willis->mouse_grab == false)
			{
				// this event is an asynchronous movement notification: it does not
				// preserve the value of the mouse position at the time of emission
				// (mouseLocation always holds the current mouse coordinates)
				//
				// because of this we simply use the method from NSWindow instead
				// (since it returns local coordinates) instead of taking the
				// screen-space value available in NSEvent
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
			}
			else
			{
				double diff_x = willis_msg_float(
					event,
					sel_getUid("deltaX"));

				double diff_y = willis_msg_float(
					event,
					sel_getUid("deltaY"));

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

				event_code = WILLIS_MOUSE_MOTION;
				event_state = WILLIS_STATE_NONE;
				willis->diff_x = diff_x;
				willis->diff_y = diff_y;
			}

			break;
		}
		case NSEventTypeKeyDown:
		{
			bool repeat = willis_msg_bool(
				event,
				sel_getUid("isARepeat"));

			if (repeat == false)
			{
				uint8_t code = willis_msg_u16(
					event,
					sel_getUid("keyCode"));

				event_state = WILLIS_STATE_PRESS;
				event_code = apple_keycode_table[code];
			}

			id string = willis_msg_id(
				event,
				sel_getUid("characters"));

			const char* str = willis_msg_utf8(
				string,
				sel_getUid("UTF8String"));

			willis->utf8_string = strdup(str);
			willis->utf8_size = strlen(str);

			break;
		}
		case NSEventTypeKeyUp:
		{
			bool repeat = willis_msg_bool(
				event,
				sel_getUid("isARepeat"));

			if (repeat == false)
			{
				uint8_t code = willis_msg_u16(
					event,
					sel_getUid("keyCode"));

				event_state = WILLIS_STATE_RELEASE;
				event_code = apple_keycode_table[code];
			}

			break;
		}
		case NSEventTypeFlagsChanged:
		{
			enum NSEventModifierFlags flags = willis_msg_type(
				event,
				sel_getUid("modifierFlags"));

			uint8_t code = willis_msg_u16(
				event,
				sel_getUid("keyCode"));

			if ((flags & NSEventModifierFlagCapsLock)
				!= (willis->quartz_old_flags & NSEventModifierFlagCapsLock))
			{
				willis->callback(
					willis,
					apple_keycode_table[code],
					WILLIS_STATE_PRESS,
					willis->data);

				willis->quartz_capslock = true;
			}

			send_system_keys(willis, flags, NSEventModifierFlagShift, code);
			send_system_keys(willis, flags, NSEventModifierFlagControl, code);
			send_system_keys(willis, flags, NSEventModifierFlagOption, code);
			send_system_keys(willis, flags, NSEventModifierFlagCommand, code);

			willis->quartz_old_flags = flags;

			return;
		}
		case NSEventTypeSystemDefined:
		{
			if (willis->quartz_capslock == true)
			{
				event_state = WILLIS_STATE_RELEASE;
				event_code = apple_keycode_table[kVK_CapsLock];

				willis->quartz_capslock = false;
			}

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

bool willis_mouse_grab(struct willis* willis)
{
	CGAssociateMouseAndMouseCursorPosition(false);
	CGDisplayHideCursor(kCGDirectMainDisplay);
	willis->mouse_grab = true;

	return true;
}

bool willis_mouse_ungrab(struct willis* willis)
{
	CGDisplayShowCursor(kCGDirectMainDisplay);
	CGAssociateMouseAndMouseCursorPosition(true);
	willis->mouse_grab = false;

	return true;
}

bool willis_free(struct willis* willis)
{
	return true;
}
