#include "willis.h"

#include "willis_events.h"

#include <stdint.h>
#include <stdbool.h>

#include <xcb/xcb.h>

// sparse LUT for x11 keycode translation
enum willis_event_code x11_keycode_table[256] =
{
[  9] = WILLIS_KEY_ESCAPE,
		WILLIS_KEY_1,
		WILLIS_KEY_2,
		WILLIS_KEY_3,
		WILLIS_KEY_4,
		WILLIS_KEY_5,
		WILLIS_KEY_6,
		WILLIS_KEY_7,
		WILLIS_KEY_8,
		WILLIS_KEY_9,
		WILLIS_KEY_0,
		WILLIS_KEY_HYPHEN_MINUS,
		WILLIS_KEY_EQUALS,
		WILLIS_KEY_BACKSPACE,
		WILLIS_KEY_TAB,
		WILLIS_KEY_Q,
		WILLIS_KEY_W,
		WILLIS_KEY_E,
		WILLIS_KEY_R,
		WILLIS_KEY_T,
		WILLIS_KEY_Y,
		WILLIS_KEY_U,
		WILLIS_KEY_I,
		WILLIS_KEY_O,
		WILLIS_KEY_P,
		WILLIS_KEY_BRACKET_LEFT,
		WILLIS_KEY_BRACKET_RIGHT,
		WILLIS_KEY_ENTER,
		WILLIS_KEY_CTRL_LEFT,
		WILLIS_KEY_A,
		WILLIS_KEY_S,
		WILLIS_KEY_D,
		WILLIS_KEY_F,
		WILLIS_KEY_G,
		WILLIS_KEY_H,
		WILLIS_KEY_J,
		WILLIS_KEY_K,
		WILLIS_KEY_L,
		WILLIS_KEY_SEMICOLON,
		WILLIS_KEY_APOSTROPHE,
		WILLIS_KEY_GRAVE,
		WILLIS_KEY_SHIFT_LEFT,
		WILLIS_KEY_ANTISLASH,
		WILLIS_KEY_Z,
		WILLIS_KEY_X,
		WILLIS_KEY_C,
		WILLIS_KEY_V,
		WILLIS_KEY_B,
		WILLIS_KEY_N,
		WILLIS_KEY_M,
		WILLIS_KEY_COMMA,
		WILLIS_KEY_PERIOD,
		WILLIS_KEY_SLASH,
		WILLIS_KEY_SHIFT_RIGHT,
		WILLIS_KEY_NUM_ASTERISK,
		WILLIS_KEY_ALT_LEFT,
		WILLIS_KEY_SPACE,
		WILLIS_KEY_CAPS_LOCK,
		WILLIS_KEY_F1,
		WILLIS_KEY_F2,
		WILLIS_KEY_F3,
		WILLIS_KEY_F4,
		WILLIS_KEY_F5,
		WILLIS_KEY_F6,
		WILLIS_KEY_F7,
		WILLIS_KEY_F8,
		WILLIS_KEY_F9,
		WILLIS_KEY_F10,
		WILLIS_KEY_NUM_LOCK,
		WILLIS_KEY_SCROLL_LOCK,
		WILLIS_KEY_NUM_7,
		WILLIS_KEY_NUM_8,
		WILLIS_KEY_NUM_9,
		WILLIS_KEY_NUM_HYPHEN_MINUS,
		WILLIS_KEY_NUM_4,
		WILLIS_KEY_NUM_5,
		WILLIS_KEY_NUM_6,
		WILLIS_KEY_NUM_PLUS,
		WILLIS_KEY_NUM_1,
		WILLIS_KEY_NUM_2,
		WILLIS_KEY_NUM_3,
		WILLIS_KEY_NUM_0,
		WILLIS_KEY_NUM_DELETE,
[ 95] = WILLIS_KEY_F11,
		WILLIS_KEY_F12,
[104] = WILLIS_KEY_NUM_ENTER,
		WILLIS_KEY_CTRL_RIGHT,
		WILLIS_KEY_NUM_SLASH,
		WILLIS_KEY_PRINT_SCREEN,
		WILLIS_KEY_ALT_RIGHT,
[110] = WILLIS_KEY_HOME,
		WILLIS_KEY_UP,
		WILLIS_KEY_PAGE_UP,
		WILLIS_KEY_LEFT,
		WILLIS_KEY_RIGHT,
		WILLIS_KEY_END,
		WILLIS_KEY_DOWN,
		WILLIS_KEY_PAGE_DOWN,
		WILLIS_KEY_INSERT,
		WILLIS_KEY_DELETE,
[127] = WILLIS_KEY_PAUSE,
[133] = WILLIS_KEY_MOD_LEFT,
		WILLIS_KEY_MOD_RIGHT,
		WILLIS_KEY_MENU,
};

// we must use the static qualifier here because the linker refuses to find the
// functions from another translation unit otherwise (like in dragonfail)
static inline enum willis_event_code willis_translate_keycode_x11(
	xcb_keycode_t keycode)
{
	// xcb_keycode_t is a u8, so it shoudn't be able to trigger a buffer overflow
	// TODO add static test in pre-processor (if possible, must investigate)
	return x11_keycode_table[keycode];
}

// ditto...
static inline enum willis_event_code willis_translate_button_x11(
	xcb_button_t button)
{
	switch (button)
	{
		case XCB_BUTTON_INDEX_1:
		{
			return WILLIS_MOUSE_CLICK_LEFT;
		}
		case XCB_BUTTON_INDEX_2:
		{
			return WILLIS_MOUSE_CLICK_RIGHT;
		}
		case XCB_BUTTON_INDEX_3:
		{
			return WILLIS_MOUSE_CLICK_MIDDLE;
		}
		case XCB_BUTTON_INDEX_4:
		{
			return WILLIS_MOUSE_WHEEL_UP;
		}
		case XCB_BUTTON_INDEX_5:
		{
			return WILLIS_MOUSE_WHEEL_DOWN;
		}
		default:
		{
			return WILLIS_NONE;
		}
	}
}

void willis_keyboard_utf8(
	struct willis* willis,
	enum willis_event_code event_code,
	char* out)
{

}

void willis_init(
	struct willis* willis,
	union willis_display_system fd,
	void (*callback)(
		struct willis* willis,
		enum willis_event_code event_code,
		enum willis_event_state event_state,
		void* data),
	void* data)
{
	// willis init
	willis->fd = fd;
	willis->callback = callback;
	willis->data = data;
}

void willis_handle_events(
	void* event,
	void* willis)
{
	xcb_generic_event_t* xcb_event;
	enum willis_event_code event_code;
	enum willis_event_state event_state;

	xcb_event = (xcb_generic_event_t*) event;

	switch (xcb_event->response_type & ~0x80)
	{
		case XCB_KEY_PRESS:
		{
			xcb_key_press_event_t* key_press;
			key_press = (xcb_key_press_event_t*) event;
			event_code = willis_translate_keycode_x11(key_press->detail);
			event_state = WILLIS_STATE_PRESS;

			break;
		}
		case XCB_KEY_RELEASE:
		{
			xcb_key_release_event_t* key_release;
			key_release = (xcb_key_release_event_t*) event;
			event_code = willis_translate_keycode_x11(key_release->detail);
			event_state = WILLIS_STATE_RELEASE;

			break;
		}
		case XCB_BUTTON_PRESS:
		{
			xcb_button_press_event_t* button_press;
			button_press = (xcb_button_press_event_t*) event;
			event_code = willis_translate_button_x11(button_press->detail);
			event_state = WILLIS_STATE_PRESS;

			break;
		}
		case XCB_BUTTON_RELEASE:
		{
			xcb_button_release_event_t* button_release;
			button_release = (xcb_button_release_event_t*) event;
			event_code = willis_translate_button_x11(button_release->detail);
			event_state = WILLIS_STATE_RELEASE;

			break;
		}
		case XCB_MOTION_NOTIFY:
		{
			// TODO save coordinates
#if 0
			xcb_motion_notify_event_t* motion;
			motion = (xcb_motion_notify_event_t*) event;
#endif
			event_code = WILLIS_MOUSE_MOTION;
			event_state = WILLIS_STATE_NONE;

			break;
		}
	}

	// execute user callback
	if (event_code != WILLIS_NONE)
	{
		struct willis* ctx = (struct willis*) willis;
		ctx->callback(ctx, event_code, event_state, ctx->data);
	}
}
