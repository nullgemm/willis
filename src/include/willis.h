#ifndef H_WILLIS
#define H_WILLIS

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct willis;

enum willis_error
{
	WILLIS_ERROR_OK = 0,
	WILLIS_ERROR_NULL,
	WILLIS_ERROR_ALLOC,
	WILLIS_ERROR_BOUNDS,
	WILLIS_ERROR_DOMAIN,
	WILLIS_ERROR_FD,

	WILLIS_ERROR_EVENT_CODE_INVALID,
	WILLIS_ERROR_EVENT_STATE_INVALID,

	WILLIS_ERROR_X11_XFIXES_VERSION,
	WILLIS_ERROR_X11_XFIXES_HIDE,
	WILLIS_ERROR_X11_XFIXES_SHOW,
	WILLIS_ERROR_X11_GRAB,
	WILLIS_ERROR_X11_UNGRAB,
	WILLIS_ERROR_X11_XINPUT_SELECT_EVENTS,
	WILLIS_ERROR_X11_XINPUT_GET_POINTER,
	WILLIS_ERROR_X11_XKB_SETUP,
	WILLIS_ERROR_X11_XKB_DEVICE_GET,
	WILLIS_ERROR_X11_XKB_KEYMAP_NEW,
	WILLIS_ERROR_X11_XKB_STATE_NEW,
	WILLIS_ERROR_X11_XKB_SELECT_EVENTS,
	WILLIS_ERROR_XKB_CONTEXT_NEW,

	WILLIS_ERROR_WIN_MOUSE_GRAB,
	WILLIS_ERROR_WIN_MOUSE_UNGRAB,
	WILLIS_ERROR_WIN_WINDOW_RECT_GET,
	WILLIS_ERROR_WIN_WINDOW_CURSOR_CLIP,
	WILLIS_ERROR_WIN_WINDOW_CURSOR_UNCLIP,
	WILLIS_ERROR_WIN_WINDOW_MOUSE_RAW_GET,
	WILLIS_ERROR_WIN_ACTIVE_GET,

	WILLIS_ERROR_WAYLAND_REQUEST,
	WILLIS_ERROR_WAYLAND_LISTENER_ADD,
	WILLIS_ERROR_WAYLAND_POINTER_MISSING,
	WILLIS_ERROR_WAYLAND_POINTER_SURFACE_MISSING,
	WILLIS_ERROR_WAYLAND_POINTER_RELATIVE_MANAGER_MISSING,
	WILLIS_ERROR_WAYLAND_POINTER_CONSTRAINTS_MANAGER_MISSING,
	WILLIS_ERROR_WAYLAND_POINTER_RELATIVE_MISSING,
	WILLIS_ERROR_WAYLAND_POINTER_LOCKED_MISSING,
	WILLIS_ERROR_WAYLAND_POINTER_RELATIVE_GET,
	WILLIS_ERROR_WAYLAND_POINTER_LOCKED_GET,
	WILLIS_ERROR_WAYLAND_POINTER_GET,
	WILLIS_ERROR_WAYLAND_KEYBOARD_GET,

	WILLIS_ERROR_COUNT,
};

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

	WILLIS_CODE_COUNT,
};

enum willis_event_state
{
	// movements, scrolling
	WILLIS_STATE_NONE = 0,

	// clicks, typing
	WILLIS_STATE_PRESS,
	WILLIS_STATE_RELEASE,

	WILLIS_STATE_COUNT,
};

struct willis_error_info
{
	enum willis_error code;
	const char* file;
	unsigned line;
};

struct willis_event_info
{
	// willis event
	enum willis_event_code event_code;
	enum willis_event_state event_state;

	// utf-8 input string for key events
	char* utf8_string;
	size_t utf8_size;

	// mouse wheel
	int mouse_wheel_steps;

	// cursor position info for mouse events
	int mouse_x;
	int mouse_y;
	int64_t diff_x; // signed fixed-point (Q31.32)
	int64_t diff_y; // signed fixed-point (Q31.32)
};

struct willis_config_backend
{
	void* data;

	void (*init)(
		struct willis* context,
		struct willis_error_info* error);

	void (*start)(
		struct willis* context,
		void* data,
		struct willis_error_info* error);

	void (*handle_event)(
		struct willis* context,
		void* event,
		struct willis_event_info* event_info,
		struct willis_error_info* error);

	bool (*mouse_grab)(
		struct willis* context,
		struct willis_error_info* error);

	bool (*mouse_ungrab)(
		struct willis* context,
		struct willis_error_info* error);

	void (*stop)(
		struct willis* context,
		struct willis_error_info* error);

	void (*clean)(
		struct willis* context,
		struct willis_error_info* error);
};

struct willis* willis_init(
	struct willis_config_backend* config,
	struct willis_error_info* error);

void willis_start(
	struct willis* context,
	void* data,
	struct willis_error_info* error);

void willis_handle_event(
	struct willis* context,
	void* event,
	struct willis_event_info* event_info,
	struct willis_error_info* error);

const char* willis_get_event_code_name(
	struct willis* context,
	enum willis_event_code event_code,
	struct willis_error_info* error);

const char* willis_get_event_state_name(
	struct willis* context,
	enum willis_event_state event_state,
	struct willis_error_info* error);

bool willis_mouse_grab(
	struct willis* context,
	struct willis_error_info* error);

bool willis_mouse_ungrab(
	struct willis* context,
	struct willis_error_info* error);

void willis_stop(
	struct willis* context,
	struct willis_error_info* error);

void willis_clean(
	struct willis* context,
	struct willis_error_info* error);

void willis_error_log(
	struct willis* context,
	struct willis_error_info* error);

const char* willis_error_get_msg(
	struct willis* context,
	struct willis_error_info* error);

enum willis_error willis_error_get_code(
	struct willis_error_info* error);

const char* willis_error_get_file(
	struct willis_error_info* error);

unsigned willis_error_get_line(
	struct willis_error_info* error);

void willis_error_ok(
	struct willis_error_info* error);

#endif
