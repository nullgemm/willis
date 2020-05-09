#include "willis.h"

#include "willis_events.h"
#include "xkb.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <xcb/xcb.h>
#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon.h>
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
	willis->display_system = (xcb_connection_t*) backend_link;
	willis->callback = callback;
	willis->data = data;

	willis->utf8_string = NULL;
	willis->utf8_size = 0;
	willis->get_utf8 = utf8;

	// set locale for xkb composition
	willis_xkb_init_locale(willis);

	// initialize xkb
	int ok;

	ok = xkb_x11_setup_xkb_extension(
		backend_link,
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

	willis_xkb_init_compose(willis);

	willis->xkb_device_id =
		xkb_x11_get_core_keyboard_device_id(
			backend_link);

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
		case XCB_BUTTON_INDEX_3:
		{
			return WILLIS_MOUSE_CLICK_RIGHT;
		}
		case XCB_BUTTON_INDEX_2:
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

bool willis_free(struct willis* willis)
{
	if (willis->xkb_state != NULL)
	{
		xkb_state_unref(willis->xkb_state);
	}

	if (willis->xkb_keymap != NULL)
	{
		xkb_keymap_unref(willis->xkb_keymap);
	}

	if (willis->xkb_compose_table != NULL)
	{
		xkb_compose_table_unref(willis->xkb_compose_table);
	}

	if (willis->xkb_compose_state != NULL)
	{
		xkb_compose_state_unref(willis->xkb_compose_state);
	}

	if (willis->xkb_ctx != NULL)
	{
		xkb_context_unref(willis->xkb_ctx);
	}

	return true;
}
