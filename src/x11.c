#include "willis.h"
#include "willis_events.h"
#include "willis_x11.h"
#include "willis_xkb.h"
#include "xkb.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xfixes.h>
#include <xcb/xinput.h>
#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon-x11.h>

// we will use pointer aliasing with this custom structure
// to add the missing `mask` field to `xcb_input_event_mask_t`
// instead of using the misleading (but common) packing hack thing
struct willis_xinput_event_mask
{
	xcb_input_device_id_t deviceid;
	uint16_t mask_len;
	// lol who needs padding
	uint32_t mask;
};

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
	struct willis_x11* willis_x11 = &(willis->willis_x11);
	struct willis_xkb* willis_xkb = &(willis->willis_xkb);

	struct xkb_keymap* xkb_keymap =
		xkb_x11_keymap_new_from_device(
			willis_xkb->xkb_ctx,
			willis_x11->display_system,
			willis_x11->xkb_device_id,
			XKB_KEYMAP_COMPILE_NO_FLAGS);

	if (xkb_keymap == NULL)
	{
		return false;
	}

	struct xkb_state* xkb_state =
		xkb_x11_state_new_from_device(
			xkb_keymap,
			willis_x11->display_system,
			willis_x11->xkb_device_id);

	if (xkb_state == NULL)
	{
		xkb_keymap_unref(xkb_keymap);
		return false;
	}

	xkb_state_unref(willis_xkb->xkb_state);
	xkb_keymap_unref(willis_xkb->xkb_keymap);
	willis_xkb->xkb_keymap = xkb_keymap;
	willis_xkb->xkb_state = xkb_state;

	return true;
}

// from here, static functions are used in the public init function

static bool willis_select_events(struct willis* willis)
{
	struct willis_x11* willis_x11 = &(willis->willis_x11);

	// x11 variable naming madness
	willis_x11->select_events_details.affectNewKeyboard =
		XCB_XKB_NKN_DETAIL_KEYCODES;

	willis_x11->select_events_details.affectState =
		XCB_XKB_STATE_PART_MODIFIER_BASE
		| XCB_XKB_STATE_PART_MODIFIER_LATCH
		| XCB_XKB_STATE_PART_MODIFIER_LOCK
		| XCB_XKB_STATE_PART_GROUP_BASE
		| XCB_XKB_STATE_PART_GROUP_LATCH
		| XCB_XKB_STATE_PART_GROUP_LOCK;

	// this is only executed once, let's just copy
	willis_x11->select_events_details.newKeyboardDetails =
		willis_x11->select_events_details.affectNewKeyboard;

	willis_x11->select_events_details.stateDetails =
		willis_x11->select_events_details.affectState;

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
		willis_x11->display_system,
		willis_x11->xkb_device_id,
		events,
		0,
		0,
		map_parts,
		map_parts,
		&(willis_x11->select_events_details));

	error = xcb_request_check(
		willis_x11->display_system,
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
	struct willis_x11* willis_x11 = &(willis->willis_x11);
	struct willis_xkb* willis_xkb = &(willis->willis_xkb);
	struct willis_data_x11* willis_data = (void*) backend_link;

	willis_x11->display_system = willis_data->x11_conn;
	willis_x11->x11_root = willis_data->x11_root;
	willis_x11->x11_window = willis_data->x11_window;

	willis->callback = callback;
	willis->data = data;
	willis->get_utf8 = utf8;
	willis->utf8_string = NULL;
	willis->utf8_size = 0;
	willis->mouse_grab = false;
	willis->mouse_x = 0;
	willis->mouse_y = 0;
	willis->diff_x = 0;
	willis->diff_y = 0;

	// set locale for xkb composition
	willis_xkb_init_locale(willis);

	// initialize xkb
	int ok;

	ok = xkb_x11_setup_xkb_extension(
		willis_x11->display_system,
		XKB_X11_MIN_MAJOR_XKB_VERSION,
		XKB_X11_MIN_MINOR_XKB_VERSION,
		XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
		NULL,
		NULL,
		&(willis_x11->xkb_event),
		NULL);

	if (ok == 0)
	{
		return false;
	}

	willis_xkb->xkb_ctx =
		xkb_context_new(
			XKB_CONTEXT_NO_FLAGS);

	if (willis_xkb->xkb_ctx == NULL)
	{
		return false;
	}

	willis_xkb_init_compose(willis);

	willis_x11->xkb_device_id =
		xkb_x11_get_core_keyboard_device_id(
			willis_x11->display_system);

	if (willis_x11->xkb_device_id == -1)
	{
		xkb_compose_state_unref(willis_xkb->xkb_compose_state); // ok to unref if NULL
		xkb_compose_table_unref(willis_xkb->xkb_compose_table); // ok to unref if NULL
		xkb_context_unref(willis_xkb->xkb_ctx);
		return false;
	}

	bool keymap_update;
	willis_xkb->xkb_keymap = NULL;
	willis_xkb->xkb_state = NULL;

	keymap_update =
		willis_update_keymap_x11(
			willis);

	if (keymap_update == false)
	{
		xkb_compose_state_unref(willis_xkb->xkb_compose_state);
		xkb_compose_table_unref(willis_xkb->xkb_compose_table);
		xkb_context_unref(willis_xkb->xkb_ctx);
		return false;
	}

	bool event_selection;

	event_selection =
		willis_select_events(
			willis);

	if (event_selection == false)
	{
		xkb_state_unref(willis_xkb->xkb_state);
		xkb_keymap_unref(willis_xkb->xkb_keymap);
		xkb_compose_state_unref(willis_xkb->xkb_compose_state);
		xkb_compose_table_unref(willis_xkb->xkb_compose_table);
		xkb_context_unref(willis_xkb->xkb_ctx);
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
	struct willis_x11* willis_x11 = &(willis->willis_x11);
	struct willis_xkb* willis_xkb = &(willis->willis_xkb);
	union willis_magic_xkb_event* xkb_event;

	xkb_event = (union willis_magic_xkb_event*) event;

	if (xkb_event->magic.device_id != willis_x11->xkb_device_id)
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
				willis_xkb->xkb_state,
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
	struct willis_x11* willis_x11 = &(willis->willis_x11);
	struct willis_xkb* willis_xkb = &(willis->willis_xkb);
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
				if (willis_xkb->xkb_compose_state != NULL)
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
		case XCB_GE_GENERIC:
		{

			xcb_ge_generic_event_t* generic;
			generic = (xcb_ge_generic_event_t*) event;

			if (generic->event_type != XCB_INPUT_RAW_MOTION)
			{
				break;
			}

			xcb_input_raw_motion_event_t* raw;
			raw = (xcb_input_raw_motion_event_t*) event;

			int len =
				xcb_input_raw_button_press_axisvalues_length(raw);

			xcb_input_fp3232_t* axis =
				xcb_input_raw_button_press_axisvalues_raw(raw);

			xcb_input_fp3232_t value;

			if (len > 0)
			{
				value = axis[0];
				willis->diff_x = (((int64_t) value.integral) << 32) | value.frac;
			}

			if (len > 1)
			{
				value = axis[1];
				willis->diff_y = (((int64_t) value.integral) << 32) | value.frac;
			}

			event_code = WILLIS_MOUSE_MOTION;
			event_state = WILLIS_STATE_NONE;

			break;
		}
		default:
		{
			if (event_masked == willis_x11->xkb_event)
			{
				willis_handle_xkb(willis, event);
			}

			break;
		}
	}

	// execute user callback
	if ((willis->callback != NULL) && (event_code != WILLIS_NONE))
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

static bool select_events(struct willis* willis, uint32_t mask)
{
	struct willis_x11* willis_x11 = &(willis->willis_x11);
	xcb_generic_error_t* error;

	// get the xinput device id for the pointer
	xcb_input_xi_get_client_pointer_cookie_t cookie_pointer;
	xcb_input_xi_get_client_pointer_reply_t* reply_pointer;

	cookie_pointer = xcb_input_xi_get_client_pointer(
		willis_x11->display_system,
		willis_x11->x11_window);

	reply_pointer = xcb_input_xi_get_client_pointer_reply(
		willis_x11->display_system,
		cookie_pointer,
		&error);

	if (error != NULL)
	{
		if (reply_pointer != NULL)
		{
			free(reply_pointer);
		}

		return false;
	}

	xcb_input_device_id_t dev = reply_pointer->deviceid;

	if (reply_pointer != NULL)
	{
		free(reply_pointer);
	}

	// register event
	struct willis_xinput_event_mask mask_grab =
	{
		.deviceid = dev,
		.mask_len = 1,
		.mask = mask,
	};

	xcb_input_xi_select_events(
		willis_x11->display_system,
		willis_x11->x11_root,
		1,
		(xcb_input_event_mask_t*) &mask_grab);

	if (mask == 0)
	{
		willis->mouse_grab = false;
	}
	else
	{
		willis->mouse_grab = true;
	}

	return true;
}

bool willis_mouse_grab(struct willis* willis)
{
	struct willis_x11* willis_x11 = &(willis->willis_x11);

	if (willis->mouse_grab == true)
	{
		return false;
	}

	xcb_xfixes_query_version(
		willis_x11->display_system,
		4,
		0);

	xcb_xfixes_hide_cursor(
		willis_x11->display_system,
		willis_x11->x11_window);

	xcb_grab_pointer(
		willis_x11->display_system,
		true,
		willis_x11->x11_root,
		XCB_NONE,
		XCB_GRAB_MODE_ASYNC,
		XCB_GRAB_MODE_ASYNC,
		willis_x11->x11_window,
		XCB_CURSOR_NONE, // TODO use invisible cursor
		XCB_CURRENT_TIME);

	return select_events(willis, XCB_INPUT_XI_EVENT_MASK_RAW_MOTION);
}

bool willis_mouse_ungrab(struct willis* willis)
{
	struct willis_x11* willis_x11 = &(willis->willis_x11);

	if (willis->mouse_grab == false)
	{
		return false;
	}

	xcb_ungrab_pointer(
		willis_x11->display_system,
		XCB_CURRENT_TIME);

	xcb_xfixes_show_cursor(
		willis_x11->display_system,
		willis_x11->x11_window);

	return select_events(willis, 0);
}

bool willis_free(struct willis* willis)
{
	struct willis_xkb* willis_xkb = &(willis->willis_xkb);

	if (willis_xkb->xkb_state != NULL)
	{
		xkb_state_unref(willis_xkb->xkb_state);
	}

	if (willis_xkb->xkb_keymap != NULL)
	{
		xkb_keymap_unref(willis_xkb->xkb_keymap);
	}

	if (willis_xkb->xkb_compose_table != NULL)
	{
		xkb_compose_table_unref(willis_xkb->xkb_compose_table);
	}

	if (willis_xkb->xkb_compose_state != NULL)
	{
		xkb_compose_state_unref(willis_xkb->xkb_compose_state);
	}

	if (willis_xkb->xkb_ctx != NULL)
	{
		xkb_context_unref(willis_xkb->xkb_ctx);
	}

	return true;
}
