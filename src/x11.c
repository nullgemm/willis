#include "willis.h"

#include "willis_events.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <locale.h>
#include <xcb/xcb.h>
#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon-compose.h>

// universal event structure compatible with all xkb events
// this is actually used in the code example provided with xkb
// it is a hack used to process xkb configuration changes quickly
struct willis_universal_xkb_event
{
	uint8_t response_type;
	uint8_t xkb_type;
	uint16_t sequence;
	xcb_timestamp_t time;
	uint8_t device_id;
};

union willis_magic_xkb_event
{
	struct willis_universal_xkb_event magic;
	xcb_xkb_new_keyboard_notify_event_t keyboard;
	xcb_xkb_state_notify_event_t state;
	xcb_xkb_map_notify_event_t map;
};

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

// from here, static functions are used in the public init and handle functions

static bool willis_update_keymap_x11(struct willis* willis)
{
	struct xkb_keymap* xkb_keymap =
		xkb_x11_keymap_new_from_device(
			willis->xkb_ctx,
			willis->display_system,
			willis->xkb_device_id,
			XKB_KEYMAP_COMPILE_NO_FLAGS);

	if (xkb_keymap == NULL)
	{
		return false;
	}

	struct xkb_state* xkb_state =
		xkb_x11_state_new_from_device(
			xkb_keymap,
			willis->display_system,
			willis->xkb_device_id);

	if (xkb_state == NULL)
	{
		xkb_keymap_unref(xkb_keymap);
		return false;
	}

	xkb_state_unref(willis->xkb_state);
	xkb_keymap_unref(willis->xkb_keymap);
	willis->xkb_keymap = xkb_keymap;
	willis->xkb_state = xkb_state;

	return true;
}

// from here, static functions are used in the public init function

static bool willis_select_events(struct willis* willis)
{
	// x11 variable naming madness
	willis->select_events_details.affectNewKeyboard =
		XCB_XKB_NKN_DETAIL_KEYCODES;

	willis->select_events_details.affectState =
		XCB_XKB_STATE_PART_MODIFIER_BASE
		| XCB_XKB_STATE_PART_MODIFIER_LATCH
		| XCB_XKB_STATE_PART_MODIFIER_LOCK
		| XCB_XKB_STATE_PART_GROUP_BASE
		| XCB_XKB_STATE_PART_GROUP_LATCH
		| XCB_XKB_STATE_PART_GROUP_LOCK;

	// this is only execute once, let's just copy
	willis->select_events_details.newKeyboardDetails =
		willis->select_events_details.affectNewKeyboard;

	willis->select_events_details.stateDetails =
		willis->select_events_details.affectState;

	// more of it
	uint16_t events =
		XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY
		| XCB_XKB_EVENT_TYPE_MAP_NOTIFY
		| XCB_XKB_EVENT_TYPE_STATE_NOTIFY;

	uint16_t map_parts = 
		XCB_XKB_MAP_PART_KEY_TYPES
		| XCB_XKB_MAP_PART_KEY_SYMS
		| XCB_XKB_MAP_PART_MODIFIER_MAP
		| XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS
		| XCB_XKB_MAP_PART_KEY_ACTIONS
		| XCB_XKB_MAP_PART_VIRTUAL_MODS
		| XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP;

	xcb_void_cookie_t cookie;
	xcb_generic_error_t* error;

	// classic xcb function with 321948571 parameters
	cookie = xcb_xkb_select_events_aux_checked(
		willis->display_system,
		willis->xkb_device_id,
		events,
		0,
		0,
		map_parts,
		map_parts,
		&(willis->select_events_details));

	error = xcb_request_check(
		willis->display_system,
		cookie);

	if (error != NULL)
	{
		free(error);

		return false;
	}

	return true;
}

static bool null_or_empty(const char* str)
{
	return (str == NULL) || ((*str) == '\0');
}

bool willis_init(
	struct willis* willis,
	void* display_system,
	bool utf8,
	void (*callback)(
		struct willis* willis,
		enum willis_event_code event_code,
		enum willis_event_state event_state,
		void* data),
	void* data)
{
	willis->display_system = (xcb_connection_t*) display_system;
	willis->callback = callback;
	willis->data = data;

	willis->utf8_string = NULL;
	willis->utf8_size = 0;
	willis->get_utf8 = utf8;

	// set locale for xkb composition
	const char* locale = getenv("LC_ALL");

	if (null_or_empty(locale) == true)
	{
		locale = getenv("LC_CTYPE");

		if (null_or_empty(locale) == true)
		{
			locale = getenv("LANG");

			if (null_or_empty(locale) == true)
			{
				locale = "C";
			}
		}
	}

	willis->xkb_locale = locale;

	// initialize xkb
	int ok;

	ok = xkb_x11_setup_xkb_extension(
		display_system,
		XKB_X11_MIN_MAJOR_XKB_VERSION,
		XKB_X11_MIN_MINOR_XKB_VERSION,
		XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
		NULL,
		NULL,
		&(willis->xkb_event),
		NULL);

	if (ok == 0)
	{
		return false;
	}

	willis->xkb_ctx =
		xkb_context_new(
			XKB_CONTEXT_NO_FLAGS);

	if (willis->xkb_ctx == NULL)
	{
		return false;
	}

	// initialize compose table (might me NULL)
	willis->xkb_compose_table = xkb_compose_table_new_from_locale(
		willis->xkb_ctx,
		willis->xkb_locale,
		XKB_COMPOSE_COMPILE_NO_FLAGS);

	// initialize compose state (might me NULL)
	if (willis->xkb_compose_table != NULL)
	{
		willis->xkb_compose_state = xkb_compose_state_new(
			willis->xkb_compose_table,
			XKB_COMPOSE_STATE_NO_FLAGS);
	}
	else
	{
		willis->xkb_compose_state = NULL;
	}

	willis->xkb_device_id =
		xkb_x11_get_core_keyboard_device_id(
			display_system);

	if (willis->xkb_device_id == -1)
	{
		xkb_compose_state_unref(willis->xkb_compose_state); // ok to unref if NULL
		xkb_compose_table_unref(willis->xkb_compose_table); // ok to unref if NULL
		xkb_context_unref(willis->xkb_ctx);
		return false;
	}

	bool keymap_update;
	willis->xkb_keymap = NULL;
	willis->xkb_state = NULL;

	keymap_update =
		willis_update_keymap_x11(
			willis);

	if (keymap_update == false)
	{
		xkb_compose_state_unref(willis->xkb_compose_state);
		xkb_compose_table_unref(willis->xkb_compose_table);
		xkb_context_unref(willis->xkb_ctx);
		return false;
	}

	bool event_selection;

	event_selection =
		willis_select_events(
			willis);

	if (event_selection == false)
	{
		xkb_state_unref(willis->xkb_state);
		xkb_keymap_unref(willis->xkb_keymap);
		xkb_compose_state_unref(willis->xkb_compose_state);
		xkb_compose_table_unref(willis->xkb_compose_table);
		xkb_context_unref(willis->xkb_ctx);
		return false;
	}

	return true;
}

// from here, static functions are used in the public handle function

static inline enum willis_event_code willis_translate_keycode_x11(
	xcb_keycode_t keycode)
{
	// we must use the static qualifier here because the linker refuses to find
	// the functions from another translation unit otherwise

	// xcb_keycode_t is a u8, so it shoudn't be able to trigger a buffer overflow
	// TODO add static test in pre-processor (if possible, must investigate)

	return x11_keycode_table[keycode];
}

static inline enum willis_event_code willis_translate_button_x11(
	xcb_button_t button)
{
	// we must use the static qualifier here because the linker refuses to find
	// the functions from another translation unit otherwise

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

static void willis_utf8_simple(struct willis* willis, xkb_keycode_t keycode)
{
	willis->utf8_size = xkb_state_key_get_utf8(
		willis->xkb_state,
		keycode,
		NULL,
		0);

	willis->utf8_string = malloc(willis->utf8_size + 1);

	if (willis->utf8_string != NULL)
	{
		xkb_state_key_get_utf8(
			willis->xkb_state,
			keycode,
			willis->utf8_string,
			willis->utf8_size + 1);
	}
}

static void willis_utf8_compose(struct willis* willis, xkb_keycode_t keycode)
{
	// get keysym
	xkb_keysym_t keysym = xkb_state_key_get_one_sym(
		willis->xkb_state,
		keycode);

	enum xkb_compose_feed_result result = xkb_compose_state_feed(
		willis->xkb_compose_state,
		keysym);

	// compose keysym
	if (result == XKB_COMPOSE_FEED_ACCEPTED)
	{
		// get composition status
		enum xkb_compose_status status = xkb_compose_state_get_status(
			willis->xkb_compose_state);

		// print composed utf-8 value
		if (status == XKB_COMPOSE_COMPOSED)
		{
			willis->utf8_size = xkb_compose_state_get_utf8(
				willis->xkb_compose_state,
				NULL,
				0);

			willis->utf8_string = malloc(willis->utf8_size + 1);

			if (willis->utf8_string != NULL)
			{
				xkb_compose_state_get_utf8(
					willis->xkb_compose_state,
					willis->utf8_string,
					willis->utf8_size + 1);
			}
		}
		// print simple utf-8 value
		else if (status == XKB_COMPOSE_NOTHING)
		{
			willis_utf8_simple(
				willis,
				keycode);
		}
	}
}

static void willis_handle_xkb(
	struct willis* willis,
	xcb_generic_event_t* event)
{
	union willis_magic_xkb_event* xkb_event;
	xkb_event = (union willis_magic_xkb_event*) event;

	if (xkb_event->magic.device_id != willis->xkb_device_id)
	{
        return;
	}

	switch (xkb_event->magic.xkb_type)
	{
		case XCB_XKB_NEW_KEYBOARD_NOTIFY:
		{
			if ((xkb_event->keyboard.changed & XCB_XKB_NKN_DETAIL_KEYCODES) != 0)
			{
				willis_update_keymap_x11(willis);
			}

			break;
		}
		case XCB_XKB_MAP_NOTIFY:
		{
			willis_update_keymap_x11(willis);

			break;
		}
		case XCB_XKB_STATE_NOTIFY:
		{
			xkb_state_update_mask(
				willis->xkb_state,
				xkb_event->state.baseMods,
				xkb_event->state.latchedMods,
				xkb_event->state.lockedMods,
				xkb_event->state.baseGroup,
				xkb_event->state.latchedGroup,
				xkb_event->state.lockedGroup);

			break;
		}
	}
}

void willis_handle_events(
	void* event,
	void* ctx)
{
	xcb_generic_event_t* xcb_event;
	enum willis_event_code event_code = WILLIS_NONE;
	enum willis_event_state event_state;
	struct willis* willis = (struct willis*) ctx;
	uint8_t event_masked;

	xcb_event = (xcb_generic_event_t*) event;
	event_masked = xcb_event->response_type & ~0x80;

	switch (event_masked)
	{
		case XCB_KEY_PRESS:
		{
			xcb_key_press_event_t* key_press;
			key_press = (xcb_key_press_event_t*) event;
			event_code = willis_translate_keycode_x11(key_press->detail);
			event_state = WILLIS_STATE_PRESS;

			if (willis->get_utf8 == true)
			{
				// use compose functions if available
				if (willis->xkb_compose_state != NULL)
				{
					willis_utf8_compose(
						(struct willis*) willis,
						(xkb_keycode_t) key_press->detail);
				}
				// use simple keycode translation otherwise
				else
				{
					willis_utf8_simple(
						(struct willis*) willis,
						(xkb_keycode_t) key_press->detail);
				}
			}

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
			xcb_motion_notify_event_t* motion;
			motion = (xcb_motion_notify_event_t*) event;
			event_code = WILLIS_MOUSE_MOTION;
			event_state = WILLIS_STATE_NONE;

			willis->mouse_x = motion->event_x;
			willis->mouse_y = motion->event_y;

			break;
		}
		default:
		{
			if (event_masked == willis->xkb_event)
			{
				willis_handle_xkb(willis, event);
			}

			break;
		}
	}

	// execute user callback
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
