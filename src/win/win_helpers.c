#include "include/willis.h"
#include "common/willis_private.h"
#include "win/win.h"
#include "win/win_helpers.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// sparse LUT for keycode translation
static const enum willis_event_code keycode_table_win[256] =
{
[VK_BACK        ] = WILLIS_KEY_BACKSPACE,
[VK_TAB         ] = WILLIS_KEY_TAB,
[VK_RETURN      ] = WILLIS_KEY_ENTER,
[VK_SHIFT       ] = WILLIS_KEY_SHIFT_LEFT,
[VK_CONTROL     ] = WILLIS_KEY_CTRL_LEFT,
[VK_MENU        ] = WILLIS_KEY_ALT_LEFT,
#if 0
[VK_PAUSE       ] = WILLIS_KEY_PAUSE,
#endif
[VK_CAPITAL     ] = WILLIS_KEY_CAPS_LOCK,
[VK_ESCAPE      ] = WILLIS_KEY_ESCAPE,
[VK_SPACE       ] = WILLIS_KEY_SPACE,
[VK_PRIOR       ] = WILLIS_KEY_PAGE_UP,
[VK_NEXT        ] = WILLIS_KEY_PAGE_DOWN,
[VK_END         ] = WILLIS_KEY_END,
[VK_HOME        ] = WILLIS_KEY_HOME,
[VK_LEFT        ] = WILLIS_KEY_LEFT,
[VK_UP          ] = WILLIS_KEY_UP,
[VK_RIGHT       ] = WILLIS_KEY_RIGHT,
[VK_DOWN        ] = WILLIS_KEY_DOWN,
[VK_SNAPSHOT    ] = WILLIS_KEY_PRINT_SCREEN,
[VK_INSERT      ] = WILLIS_KEY_INSERT,
[VK_DELETE      ] = WILLIS_KEY_DELETE,
['0'            ] = WILLIS_KEY_0,
					WILLIS_KEY_1,
					WILLIS_KEY_2,
					WILLIS_KEY_3,
					WILLIS_KEY_4,
					WILLIS_KEY_5,
					WILLIS_KEY_6,
					WILLIS_KEY_7,
					WILLIS_KEY_8,
					WILLIS_KEY_9,
['A'            ] = WILLIS_KEY_A,
					WILLIS_KEY_B,
					WILLIS_KEY_C,
					WILLIS_KEY_D,
					WILLIS_KEY_E,
					WILLIS_KEY_F,
					WILLIS_KEY_G,
					WILLIS_KEY_H,
					WILLIS_KEY_I,
					WILLIS_KEY_J,
					WILLIS_KEY_K,
					WILLIS_KEY_L,
					WILLIS_KEY_M,
					WILLIS_KEY_N,
					WILLIS_KEY_O,
					WILLIS_KEY_P,
					WILLIS_KEY_Q,
					WILLIS_KEY_R,
					WILLIS_KEY_S,
					WILLIS_KEY_T,
					WILLIS_KEY_U,
					WILLIS_KEY_V,
					WILLIS_KEY_W,
					WILLIS_KEY_X,
					WILLIS_KEY_Y,
					WILLIS_KEY_Z,
[VK_LWIN        ] = WILLIS_KEY_MOD_LEFT,
[VK_RWIN        ] = WILLIS_KEY_MOD_RIGHT,
[VK_APPS        ] = WILLIS_KEY_MENU,
[VK_NUMPAD0     ] = WILLIS_KEY_NUM_0,
[VK_NUMPAD1     ] = WILLIS_KEY_NUM_1,
[VK_NUMPAD2     ] = WILLIS_KEY_NUM_2,
[VK_NUMPAD3     ] = WILLIS_KEY_NUM_3,
[VK_NUMPAD4     ] = WILLIS_KEY_NUM_4,
[VK_NUMPAD5     ] = WILLIS_KEY_NUM_5,
[VK_NUMPAD6     ] = WILLIS_KEY_NUM_6,
[VK_NUMPAD7     ] = WILLIS_KEY_NUM_7,
[VK_NUMPAD8     ] = WILLIS_KEY_NUM_8,
[VK_NUMPAD9     ] = WILLIS_KEY_NUM_9,
[VK_MULTIPLY    ] = WILLIS_KEY_NUM_ASTERISK,
[VK_ADD         ] = WILLIS_KEY_NUM_PLUS,
[VK_SUBTRACT    ] = WILLIS_KEY_NUM_HYPHEN_MINUS,
[VK_DIVIDE      ] = WILLIS_KEY_NUM_SLASH,
[VK_F1          ] = WILLIS_KEY_F1,
[VK_F2          ] = WILLIS_KEY_F2,
[VK_F3          ] = WILLIS_KEY_F3,
[VK_F4          ] = WILLIS_KEY_F4,
[VK_F5          ] = WILLIS_KEY_F5,
[VK_F6          ] = WILLIS_KEY_F6,
[VK_F7          ] = WILLIS_KEY_F7,
[VK_F8          ] = WILLIS_KEY_F8,
[VK_F9          ] = WILLIS_KEY_F9,
[VK_F10         ] = WILLIS_KEY_F10,
[VK_F11         ] = WILLIS_KEY_F11,
[VK_F12         ] = WILLIS_KEY_F12,
[VK_NUMLOCK     ] = WILLIS_KEY_NUM_LOCK,
#if 0
[VK_SCROLL      ] = WILLIS_KEY_SCROLL_LOCK,
#endif
[VK_OEM_1       ] = WILLIS_KEY_SEMICOLON,
[VK_OEM_PLUS    ] = WILLIS_KEY_EQUALS,
[VK_OEM_COMMA   ] = WILLIS_KEY_COMMA,
[VK_OEM_MINUS   ] = WILLIS_KEY_HYPHEN_MINUS,
[VK_OEM_PERIOD  ] = WILLIS_KEY_PERIOD,
[VK_OEM_2       ] = WILLIS_KEY_SLASH,
[VK_OEM_3       ] = WILLIS_KEY_GRAVE,
[VK_OEM_4       ] = WILLIS_KEY_BRACKET_LEFT,
[VK_OEM_5       ] = WILLIS_KEY_ANTISLASH,
[VK_OEM_6       ] = WILLIS_KEY_BRACKET_RIGHT,
[VK_OEM_7       ] = WILLIS_KEY_APOSTROPHE,
[VK_OEM_102     ] = WILLIS_KEY_ALT_RIGHT,
[VK_DECIMAL     ] = WILLIS_KEY_NUM_DELETE,
};

enum willis_event_code win_helpers_keycode_table(
	uint8_t code)
{
	return keycode_table_win[code];
}
