#include "include/willis.h"
#include "common/willis_private.h"
#include "appkit/macos.h"
#include "appkit/macos_helpers.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h> // virtual keycode enums

// sparse LUT for keycode translation
static const enum willis_event_code keycode_table_appkit[256] =
{
[kVK_ANSI_A             ] =	WILLIS_KEY_A,
[kVK_ANSI_S             ] =	WILLIS_KEY_S,
[kVK_ANSI_D             ] =	WILLIS_KEY_D,
[kVK_ANSI_F             ] =	WILLIS_KEY_F,
[kVK_ANSI_H             ] =	WILLIS_KEY_H,
[kVK_ANSI_G             ] =	WILLIS_KEY_G,
[kVK_ANSI_Z             ] =	WILLIS_KEY_Z,
[kVK_ANSI_X             ] =	WILLIS_KEY_X,
[kVK_ANSI_C             ] =	WILLIS_KEY_C,
[kVK_ANSI_V             ] =	WILLIS_KEY_V,
[kVK_ANSI_B             ] =	WILLIS_KEY_B,
[kVK_ANSI_Q             ] =	WILLIS_KEY_Q,
[kVK_ANSI_W             ] =	WILLIS_KEY_W,
[kVK_ANSI_E             ] =	WILLIS_KEY_E,
[kVK_ANSI_R             ] =	WILLIS_KEY_R,
[kVK_ANSI_Y             ] =	WILLIS_KEY_Y,
[kVK_ANSI_T             ] =	WILLIS_KEY_T,
[kVK_ANSI_1             ] =	WILLIS_KEY_1,
[kVK_ANSI_2             ] =	WILLIS_KEY_2,
[kVK_ANSI_3             ] =	WILLIS_KEY_3,
[kVK_ANSI_4             ] =	WILLIS_KEY_4,
[kVK_ANSI_6             ] =	WILLIS_KEY_6,
[kVK_ANSI_5             ] =	WILLIS_KEY_5,
[kVK_ANSI_Equal         ] =	WILLIS_KEY_EQUALS,
[kVK_ANSI_9             ] =	WILLIS_KEY_9,
[kVK_ANSI_7             ] =	WILLIS_KEY_7,
[kVK_ANSI_Minus         ] =	WILLIS_KEY_HYPHEN_MINUS,
[kVK_ANSI_8             ] =	WILLIS_KEY_8,
[kVK_ANSI_0             ] =	WILLIS_KEY_0,
[kVK_ANSI_RightBracket  ] =	WILLIS_KEY_BRACKET_RIGHT,
[kVK_ANSI_O             ] =	WILLIS_KEY_O,
[kVK_ANSI_U             ] =	WILLIS_KEY_U,
[kVK_ANSI_LeftBracket   ] =	WILLIS_KEY_BRACKET_LEFT,
[kVK_ANSI_I             ] =	WILLIS_KEY_I,
[kVK_ANSI_P             ] =	WILLIS_KEY_P,
[kVK_ANSI_L             ] =	WILLIS_KEY_L,
[kVK_ANSI_J             ] =	WILLIS_KEY_J,
[kVK_ANSI_Quote         ] =	WILLIS_KEY_APOSTROPHE,
[kVK_ANSI_K             ] =	WILLIS_KEY_K,
[kVK_ANSI_Semicolon     ] =	WILLIS_KEY_SEMICOLON,
[kVK_ANSI_Backslash     ] =	WILLIS_KEY_ANTISLASH,
[kVK_ANSI_Comma         ] =	WILLIS_KEY_COMMA,
[kVK_ANSI_Slash         ] =	WILLIS_KEY_SLASH,
[kVK_ANSI_N             ] =	WILLIS_KEY_N,
[kVK_ANSI_M             ] =	WILLIS_KEY_M,
[kVK_ANSI_Period        ] =	WILLIS_KEY_PERIOD,
[kVK_ANSI_Grave         ] =	WILLIS_KEY_GRAVE,
[kVK_ANSI_KeypadDecimal ] =	WILLIS_KEY_NUM_DELETE,
[kVK_ANSI_KeypadMultiply] =	WILLIS_KEY_NUM_ASTERISK,
[kVK_ANSI_KeypadPlus    ] =	WILLIS_KEY_NUM_PLUS,
[kVK_ANSI_KeypadClear   ] =	WILLIS_KEY_NUM_LOCK,
[kVK_ANSI_KeypadDivide  ] =	WILLIS_KEY_NUM_SLASH,
[kVK_ANSI_KeypadEnter   ] = WILLIS_KEY_NUM_ENTER,
[kVK_ANSI_KeypadMinus   ] =	WILLIS_KEY_NUM_HYPHEN_MINUS,
[kVK_ANSI_Keypad0       ] =	WILLIS_KEY_NUM_0,
[kVK_ANSI_Keypad1       ] =	WILLIS_KEY_NUM_1,
[kVK_ANSI_Keypad2       ] =	WILLIS_KEY_NUM_2,
[kVK_ANSI_Keypad3       ] =	WILLIS_KEY_NUM_3,
[kVK_ANSI_Keypad4       ] =	WILLIS_KEY_NUM_4,
[kVK_ANSI_Keypad5       ] =	WILLIS_KEY_NUM_5,
[kVK_ANSI_Keypad6       ] =	WILLIS_KEY_NUM_6,
[kVK_ANSI_Keypad7       ] = WILLIS_KEY_NUM_7,
[kVK_ANSI_Keypad8       ] =	WILLIS_KEY_NUM_8,
[kVK_ANSI_Keypad9       ] =	WILLIS_KEY_NUM_9,
[kVK_Return             ] =	WILLIS_KEY_ENTER,
[kVK_Tab                ] =	WILLIS_KEY_TAB,
[kVK_Space              ] =	WILLIS_KEY_SPACE,
[kVK_Delete             ] =	WILLIS_KEY_BACKSPACE,
[kVK_Escape             ] = WILLIS_KEY_ESCAPE,
[kVK_Command            ] = WILLIS_KEY_MOD_LEFT,
[kVK_Shift              ] =	WILLIS_KEY_SHIFT_LEFT,
[kVK_CapsLock           ] =	WILLIS_KEY_CAPS_LOCK,
[kVK_Option             ] =	WILLIS_KEY_ALT_LEFT,
[kVK_Control            ] =	WILLIS_KEY_CTRL_LEFT,
[kVK_RightCommand       ] =	WILLIS_KEY_MOD_RIGHT,
[kVK_RightShift         ] =	WILLIS_KEY_SHIFT_RIGHT,
[kVK_RightOption        ] =	WILLIS_KEY_ALT_RIGHT,
[kVK_RightControl       ] =	WILLIS_KEY_CTRL_RIGHT,
[kVK_F5                 ] =	WILLIS_KEY_F5,
[kVK_F6                 ] =	WILLIS_KEY_F6,
[kVK_F7                 ] =	WILLIS_KEY_F7,
[kVK_F3                 ] =	WILLIS_KEY_F3,
[kVK_F8                 ] =	WILLIS_KEY_F8,
[kVK_F9                 ] =	WILLIS_KEY_F9,
[kVK_F11                ] = WILLIS_KEY_F11,
[kVK_F10                ] =	WILLIS_KEY_F10,
[0x6E                   ] =	WILLIS_KEY_MENU,
[kVK_F12                ] =	WILLIS_KEY_F12,
[kVK_F13                ] =	WILLIS_KEY_PRINT_SCREEN,
[kVK_Help               ] =	WILLIS_KEY_INSERT,
[kVK_Home               ] = WILLIS_KEY_HOME,
[kVK_PageUp             ] =	WILLIS_KEY_PAGE_UP,
[kVK_ForwardDelete      ] =	WILLIS_KEY_DELETE,
[kVK_F4                 ] =	WILLIS_KEY_F4,
[kVK_End                ] =	WILLIS_KEY_END,
[kVK_F2                 ] =	WILLIS_KEY_F2,
[kVK_PageDown           ] =	WILLIS_KEY_PAGE_DOWN,
[kVK_F1                 ] =	WILLIS_KEY_F1,
[kVK_LeftArrow          ] =	WILLIS_KEY_LEFT,
[kVK_RightArrow         ] =	WILLIS_KEY_RIGHT,
[kVK_DownArrow          ] =	WILLIS_KEY_DOWN,
[kVK_UpArrow            ] =	WILLIS_KEY_UP,
#if 0
[                       ] = WILLIS_KEY_SCROLL_LOCK,
[                       ] = WILLIS_KEY_PAUSE,
#endif
};

enum willis_event_code appkit_helpers_keycode_table(
	uint8_t code)
{
	return keycode_table_appkit[code];
}

void appkit_helpers_send_system_keys(
	struct appkit_backend* backend,
	struct willis_event_info* event_info,
	NSEventModifierFlags flags,
	NSEventModifierFlags comparison,
	uint8_t code)
{
	NSEventModifierFlags lone = flags & comparison;

	if (lone != (backend->old_flags & comparison))
	{
		event_info->event_code = keycode_table_appkit[code];

		if (lone != 0)
		{
			event_info->event_state = WILLIS_STATE_PRESS;
		}
		else
		{
			event_info->event_state = WILLIS_STATE_RELEASE;
		}
	}
}
