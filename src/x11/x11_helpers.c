#include "include/willis.h"
#include "common/willis_private.h"
#include "x11/x11.h"
#include "x11/x11_helpers.h"
#include "nix/nix.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xinput.h>
#include <xkbcommon/xkbcommon-x11.h>

// HACK
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

// HACK
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


void x11_helpers_select_events_cursor(
	struct willis* context,
	uint32_t mask,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;
	xcb_generic_error_t* error_xcb = NULL;

	// get the xinput device id for the pointer
	xcb_input_xi_get_client_pointer_cookie_t cookie_pointer =
		xcb_input_xi_get_client_pointer(
			backend->conn,
			backend->window);

	xcb_input_xi_get_client_pointer_reply_t* reply_pointer =
		xcb_input_xi_get_client_pointer_reply(
			backend->conn,
			cookie_pointer,
			&error_xcb);

	if (error_xcb != NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_X11_XINPUT_GET_POINTER);
		return;
	}

	xcb_input_device_id_t dev = reply_pointer->deviceid;
	free(reply_pointer);

	// register event
	struct willis_xinput_event_mask mask_grab =
	{
		.deviceid = dev,
		.mask_len = 1,
		.mask = mask,
	};

	xcb_void_cookie_t cookie_select =
		xcb_input_xi_select_events(
			backend->conn,
			backend->root,
			1,
			(xcb_input_event_mask_t*) &mask_grab);

	error_xcb =
		xcb_request_check(
			backend->conn,
			cookie_select);

	if (error_xcb != NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_X11_XINPUT_SELECT_EVENTS);
		return;
	}

	// update internal grabbed info
	if (mask == 0)
	{
		backend->mouse_grabbed = false;
	}
	else
	{
		backend->mouse_grabbed = true;
	}

	willis_error_ok(error);
}

void x11_helpers_select_events_keyboard(
	struct willis* context,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;

	// x11 variable naming madness
	backend->xkb_select_events_details.affectNewKeyboard =
		XCB_XKB_NKN_DETAIL_KEYCODES;

	backend->xkb_select_events_details.affectState =
		XCB_XKB_STATE_PART_MODIFIER_BASE
		| XCB_XKB_STATE_PART_MODIFIER_LATCH
		| XCB_XKB_STATE_PART_MODIFIER_LOCK
		| XCB_XKB_STATE_PART_GROUP_BASE
		| XCB_XKB_STATE_PART_GROUP_LATCH
		| XCB_XKB_STATE_PART_GROUP_LOCK;

	// this is only executed once, let's just copy
	backend->xkb_select_events_details.newKeyboardDetails =
		backend->xkb_select_events_details.affectNewKeyboard;

	backend->xkb_select_events_details.stateDetails =
		backend->xkb_select_events_details.affectState;

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

	// classic xcb function with 321948571 parameters
	xcb_void_cookie_t cookie =
		xcb_xkb_select_events_aux_checked(
			backend->conn,
			backend->xkb_device_id,
			events,
			0,
			0,
			map_parts,
			map_parts,
			&(backend->xkb_select_events_details));

	xcb_generic_error_t* error_xcb =
		xcb_request_check(
			backend->conn,
			cookie);

	if (error_xcb != NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_X11_XKB_SELECT_EVENTS);
		return;
	}

	willis_error_ok(error);
}

void x11_helpers_update_keymap(
	struct willis* context,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;
	struct willis_xkb* xkb_common = backend->xkb_common;

	struct xkb_keymap* keymap =
		xkb_x11_keymap_new_from_device(
			xkb_common->context,
			backend->conn,
			backend->xkb_device_id,
			XKB_KEYMAP_COMPILE_NO_FLAGS);

	if (keymap == NULL)
	{
		willis_error_throw(context, error, WILLIS_ERROR_X11_XKB_KEYMAP_NEW);
		return;
	}

	struct xkb_state* state =
		xkb_x11_state_new_from_device(
			keymap,
			backend->conn,
			backend->xkb_device_id);

	if (state == NULL)
	{
		xkb_keymap_unref(keymap);
		willis_error_throw(context, error, WILLIS_ERROR_X11_XKB_STATE_NEW);
		return;
	}

	xkb_state_unref(xkb_common->state);
	xkb_keymap_unref(xkb_common->keymap);
	xkb_common->keymap = keymap;
	xkb_common->state = state;
	willis_error_ok(error);
}

enum willis_event_code x11_helpers_translate_button(
	xcb_button_t button)
{
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

void x11_helpers_handle_xkb(
	struct willis* context,
	xcb_generic_event_t* event,
	struct willis_error_info* error)
{
	struct x11_backend* backend = context->backend_data;
	struct willis_xkb* xkb_common = backend->xkb_common;

	// initialize error here to make the switch below more readable
	willis_error_ok(error);

	union willis_magic_xkb_event* xkb_event =
		(union willis_magic_xkb_event*) event;

	if (xkb_event->magic.device_id != backend->xkb_device_id)
	{
        return;
	}

	switch (xkb_event->magic.xkb_type)
	{
		case XCB_XKB_NEW_KEYBOARD_NOTIFY:
		{
			if ((xkb_event->keyboard.changed & XCB_XKB_NKN_DETAIL_KEYCODES) != 0)
			{
				x11_helpers_update_keymap(
					context,
					error);
			}

			break;
		}
		case XCB_XKB_MAP_NOTIFY:
		{
			x11_helpers_update_keymap(
				context,
				error);

			break;
		}
		case XCB_XKB_STATE_NOTIFY:
		{
			xkb_state_update_mask(
				xkb_common->state,
				xkb_event->state.baseMods,
				xkb_event->state.latchedMods,
				xkb_event->state.lockedMods,
				xkb_event->state.baseGroup,
				xkb_event->state.latchedGroup,
				xkb_event->state.lockedGroup);

			break;
		}
		default:
		{
			break;
		}
	}

	// error always set
}
