#ifndef H_WILLIS_INTERNAL_EVENTS
#define H_WILLIS_INTERNAL_EVENTS

// willis tells you wether keys are pressed or released because this is what
// display system events actually tell us, but it is your duty to retain
// information if you want to handle combinations
//
// while not specifically keyboard-oriented willis does however provide
// localized utf8 keycodes if you ask it to, because people happen to
// use different languages and keyboard layouts around the world
//
// knowing the character actually printed on the user's keycaps is indispensable
// to providing a functionnal UI and handling text input, so willis handles this 
// specifically because you shouldn't be ANSIing all the things like an asshole
//
// the scancodes are named after the corresponding idle-state labels on a
// traditionnal QWERTY (US) pc-104 (ANSI) keyboard (eg. Unicomp Classic 104)

enum willis_event_code
{
	// we must set this one to zero, this way sparse LUTs can help detecting
	// invalid inputs (this is used when translating X11 keycodes for example)
	WILLIS_NONE = 0,

	// state-triggered switches
	WILLIS_MOUSE_CLICK_LEFT,
	WILLIS_MOUSE_CLICK_RIGHT,
	WILLIS_MOUSE_CLICK_MIDDLE,

	// edge-triggered switches
	WILLIS_MOUSE_WHEEL_UP,
	WILLIS_MOUSE_WHEEL_DOWN,

	// position events
	WILLIS_MOUSE_MOTION,

	// important keys
	WILLIS_KEY_ESCAPE,

	// function keys
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
	WILLIS_KEY_F11,
	WILLIS_KEY_F12,

	// helpful keys
	WILLIS_KEY_GRAVE,
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
	WILLIS_KEY_ANTISLASH,

	WILLIS_KEY_CAPS_LOCK,
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
	WILLIS_KEY_ENTER,

	WILLIS_KEY_SHIFT_LEFT,
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

	WILLIS_KEY_CTRL_LEFT,
	WILLIS_KEY_MOD_LEFT,
	WILLIS_KEY_ALT_LEFT,
	WILLIS_KEY_SPACE,
	WILLIS_KEY_ALT_RIGHT,
	WILLIS_KEY_MOD_RIGHT,
	WILLIS_KEY_MENU,
	WILLIS_KEY_CTRL_RIGHT,

	// mostly useless keys
	WILLIS_KEY_PRINT_SCREEN,

#if 0
	// these do not even exist on macOS
	WILLIS_KEY_SCROLL_LOCK,
	WILLIS_KEY_PAUSE,
#endif

	WILLIS_KEY_INSERT,
	WILLIS_KEY_DELETE,
	WILLIS_KEY_HOME,
	WILLIS_KEY_END,
	WILLIS_KEY_PAGE_UP,
	WILLIS_KEY_PAGE_DOWN,

	WILLIS_KEY_UP,
	WILLIS_KEY_DOWN,
	WILLIS_KEY_LEFT,
	WILLIS_KEY_RIGHT,

	// definitely useless keys
	WILLIS_KEY_NUM_LOCK,
	WILLIS_KEY_NUM_SLASH,
	WILLIS_KEY_NUM_ASTERISK,

	WILLIS_KEY_NUM_HYPHEN_MINUS,
	WILLIS_KEY_NUM_PLUS,
	WILLIS_KEY_NUM_ENTER,
	WILLIS_KEY_NUM_DELETE,
	WILLIS_KEY_NUM_0,
	WILLIS_KEY_NUM_1,
	WILLIS_KEY_NUM_2,
	WILLIS_KEY_NUM_3,
	WILLIS_KEY_NUM_4,
	WILLIS_KEY_NUM_5,
	WILLIS_KEY_NUM_6,
	WILLIS_KEY_NUM_7,
	WILLIS_KEY_NUM_8,
	WILLIS_KEY_NUM_9,
};

enum willis_event_state
{
	// movements, scrolling
	WILLIS_STATE_NONE = 0,

	// clicks, typing
	WILLIS_STATE_PRESS,
	WILLIS_STATE_RELEASE,
};

#ifdef WILLIS_DEBUG
extern char* willis_event_code_names[111];
extern char* willis_event_state_names[3];
#endif

#endif
