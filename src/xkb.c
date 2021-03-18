#include "willis.h"
#include "willis_xkb.h"
#include "xkb.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <locale.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

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
#if 0
		WILLIS_KEY_SCROLL_LOCK,
#endif
[ 79] = WILLIS_KEY_NUM_7,
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
#if 0
[127] = WILLIS_KEY_PAUSE,
#endif
[133] = WILLIS_KEY_MOD_LEFT,
		WILLIS_KEY_MOD_RIGHT,
		WILLIS_KEY_MENU,
};

inline enum willis_event_code willis_translate_keycode_x11(uint8_t keycode)
{
	// xcb_keycode_t is a u8, so it shoudn't be able to trigger a buffer overflow
	return x11_keycode_table[keycode];
}

static bool null_or_empty(const char* str)
{
	return (str == NULL) || ((*str) == '\0');
}

void willis_xkb_init_locale(struct willis* willis)
{
	struct willis_xkb* willis_xkb = &(willis->willis_xkb);
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

	willis_xkb->xkb_locale = locale;
}

void willis_xkb_init_compose(struct willis* willis)
{
	struct willis_xkb* willis_xkb = &(willis->willis_xkb);

	// initialize compose table (might be NULL)
	willis_xkb->xkb_compose_table = xkb_compose_table_new_from_locale(
		willis_xkb->xkb_ctx,
		willis_xkb->xkb_locale,
		XKB_COMPOSE_COMPILE_NO_FLAGS);

	// initialize compose state (might be NULL)
	if (willis_xkb->xkb_compose_table != NULL)
	{
		willis_xkb->xkb_compose_state = xkb_compose_state_new(
			willis_xkb->xkb_compose_table,
			XKB_COMPOSE_STATE_NO_FLAGS);
	}
	else
	{
		willis_xkb->xkb_compose_state = NULL;
	}
}

void willis_utf8_simple(struct willis* willis, xkb_keycode_t keycode)
{
	struct willis_xkb* willis_xkb = &(willis->willis_xkb);

	if (willis_xkb->xkb_state == NULL)
	{
		return;
	}

	willis->utf8_size = xkb_state_key_get_utf8(
		willis_xkb->xkb_state,
		keycode,
		NULL,
		0);

	willis->utf8_string = malloc(willis->utf8_size + 1);

	if (willis->utf8_string != NULL)
	{
		xkb_state_key_get_utf8(
			willis_xkb->xkb_state,
			keycode,
			willis->utf8_string,
			willis->utf8_size + 1);
	}
}

void willis_utf8_compose(struct willis* willis, xkb_keycode_t keycode)
{
	struct willis_xkb* willis_xkb = &(willis->willis_xkb);

	// get keysym
	xkb_keysym_t keysym = xkb_state_key_get_one_sym(
		willis_xkb->xkb_state,
		keycode);

	enum xkb_compose_feed_result result = xkb_compose_state_feed(
		willis_xkb->xkb_compose_state,
		keysym);

	// compose keysym
	if (result == XKB_COMPOSE_FEED_ACCEPTED)
	{
		// get composition status
		enum xkb_compose_status status = xkb_compose_state_get_status(
			willis_xkb->xkb_compose_state);

		// print composed utf-8 value
		if (status == XKB_COMPOSE_COMPOSED)
		{
			willis->utf8_size = xkb_compose_state_get_utf8(
				willis_xkb->xkb_compose_state,
				NULL,
				0);

			willis->utf8_string = malloc(willis->utf8_size + 1);

			if (willis->utf8_string != NULL)
			{
				xkb_compose_state_get_utf8(
					willis_xkb->xkb_compose_state,
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
