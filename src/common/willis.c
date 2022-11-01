#include "include/willis.h"
#include "common/willis_private.h"

#include <stdlib.h>

struct willis* willis_init(
	struct willis_config_backend* config,
	struct willis_error_info* error)
{
	struct willis* context = malloc(sizeof (struct willis));

	if (context == NULL)
	{
		return NULL;
	}

	struct willis zero = {0};
	*context = zero;

	willis_error_init(context);

	context->backend_data = NULL;
	context->backend_callbacks = *config;
	context->backend_callbacks.init(context, error);

	char** code_names = context->event_code_names;
	code_names[WILLIS_NONE] =                 "WILLIS_NONE";
	code_names[WILLIS_MOUSE_CLICK_LEFT] =     "WILLIS_MOUSE_CLICK_LEFT";
	code_names[WILLIS_MOUSE_CLICK_RIGHT] =    "WILLIS_MOUSE_CLICK_RIGHT";
	code_names[WILLIS_MOUSE_CLICK_MIDDLE] =   "WILLIS_MOUSE_CLICK_MIDDLE";
	code_names[WILLIS_MOUSE_WHEEL_UP] =       "WILLIS_MOUSE_WHEEL_UP";
	code_names[WILLIS_MOUSE_WHEEL_DOWN] =     "WILLIS_MOUSE_WHEEL_DOWN";
	code_names[WILLIS_MOUSE_MOTION] =         "WILLIS_MOUSE_MOTION";
	code_names[WILLIS_KEY_ESCAPE] =           "WILLIS_KEY_ESCAPE";
	code_names[WILLIS_KEY_F1] =               "WILLIS_KEY_F1";
	code_names[WILLIS_KEY_F2] =               "WILLIS_KEY_F2";
	code_names[WILLIS_KEY_F3] =               "WILLIS_KEY_F3";
	code_names[WILLIS_KEY_F4] =               "WILLIS_KEY_F4";
	code_names[WILLIS_KEY_F5] =               "WILLIS_KEY_F5";
	code_names[WILLIS_KEY_F6] =               "WILLIS_KEY_F6";
	code_names[WILLIS_KEY_F7] =               "WILLIS_KEY_F7";
	code_names[WILLIS_KEY_F8] =               "WILLIS_KEY_F8";
	code_names[WILLIS_KEY_F9] =               "WILLIS_KEY_F9";
	code_names[WILLIS_KEY_F10] =              "WILLIS_KEY_F10";
	code_names[WILLIS_KEY_F11] =              "WILLIS_KEY_F11";
	code_names[WILLIS_KEY_F12] =              "WILLIS_KEY_F12";
	code_names[WILLIS_KEY_GRAVE] =            "WILLIS_KEY_GRAVE";
	code_names[WILLIS_KEY_1] =                "WILLIS_KEY_1";
	code_names[WILLIS_KEY_2] =                "WILLIS_KEY_2";
	code_names[WILLIS_KEY_3] =                "WILLIS_KEY_3";
	code_names[WILLIS_KEY_4] =                "WILLIS_KEY_4";
	code_names[WILLIS_KEY_5] =                "WILLIS_KEY_5";
	code_names[WILLIS_KEY_6] =                "WILLIS_KEY_6";
	code_names[WILLIS_KEY_7] =                "WILLIS_KEY_7";
	code_names[WILLIS_KEY_8] =                "WILLIS_KEY_8";
	code_names[WILLIS_KEY_9] =                "WILLIS_KEY_9";
	code_names[WILLIS_KEY_0] =                "WILLIS_KEY_0";
	code_names[WILLIS_KEY_HYPHEN_MINUS] =     "WILLIS_KEY_HYPHEN_MINUS";
	code_names[WILLIS_KEY_EQUALS] =           "WILLIS_KEY_EQUALS";
	code_names[WILLIS_KEY_BACKSPACE] =        "WILLIS_KEY_BACKSPACE";
	code_names[WILLIS_KEY_TAB] =              "WILLIS_KEY_TAB";
	code_names[WILLIS_KEY_Q] =                "WILLIS_KEY_Q";
	code_names[WILLIS_KEY_W] =                "WILLIS_KEY_W";
	code_names[WILLIS_KEY_E] =                "WILLIS_KEY_E";
	code_names[WILLIS_KEY_R] =                "WILLIS_KEY_R";
	code_names[WILLIS_KEY_T] =                "WILLIS_KEY_T";
	code_names[WILLIS_KEY_Y] =                "WILLIS_KEY_Y";
	code_names[WILLIS_KEY_U] =                "WILLIS_KEY_U";
	code_names[WILLIS_KEY_I] =                "WILLIS_KEY_I";
	code_names[WILLIS_KEY_O] =                "WILLIS_KEY_O";
	code_names[WILLIS_KEY_P] =                "WILLIS_KEY_P";
	code_names[WILLIS_KEY_BRACKET_LEFT] =     "WILLIS_KEY_BRACKET_LEFT";
	code_names[WILLIS_KEY_BRACKET_RIGHT] =    "WILLIS_KEY_BRACKET_RIGHT";
	code_names[WILLIS_KEY_ANTISLASH] =        "WILLIS_KEY_ANTISLASH";
	code_names[WILLIS_KEY_CAPS_LOCK] =        "WILLIS_KEY_CAPS_LOCK";
	code_names[WILLIS_KEY_A] =                "WILLIS_KEY_A";
	code_names[WILLIS_KEY_S] =                "WILLIS_KEY_S";
	code_names[WILLIS_KEY_D] =                "WILLIS_KEY_D";
	code_names[WILLIS_KEY_F] =                "WILLIS_KEY_F";
	code_names[WILLIS_KEY_G] =                "WILLIS_KEY_G";
	code_names[WILLIS_KEY_H] =                "WILLIS_KEY_H";
	code_names[WILLIS_KEY_J] =                "WILLIS_KEY_J";
	code_names[WILLIS_KEY_K] =                "WILLIS_KEY_K";
	code_names[WILLIS_KEY_L] =                "WILLIS_KEY_L";
	code_names[WILLIS_KEY_SEMICOLON] =        "WILLIS_KEY_SEMICOLON";
	code_names[WILLIS_KEY_APOSTROPHE] =       "WILLIS_KEY_APOSTROPHE";
	code_names[WILLIS_KEY_ENTER] =            "WILLIS_KEY_ENTER";
	code_names[WILLIS_KEY_SHIFT_LEFT] =       "WILLIS_KEY_SHIFT_LEFT";
	code_names[WILLIS_KEY_Z] =                "WILLIS_KEY_Z";
	code_names[WILLIS_KEY_X] =                "WILLIS_KEY_X";
	code_names[WILLIS_KEY_C] =                "WILLIS_KEY_C";
	code_names[WILLIS_KEY_V] =                "WILLIS_KEY_V";
	code_names[WILLIS_KEY_B] =                "WILLIS_KEY_B";
	code_names[WILLIS_KEY_N] =                "WILLIS_KEY_N";
	code_names[WILLIS_KEY_M] =                "WILLIS_KEY_M";
	code_names[WILLIS_KEY_COMMA] =            "WILLIS_KEY_COMMA";
	code_names[WILLIS_KEY_PERIOD] =           "WILLIS_KEY_PERIOD";
	code_names[WILLIS_KEY_SLASH] =            "WILLIS_KEY_SLASH";
	code_names[WILLIS_KEY_SHIFT_RIGHT] =      "WILLIS_KEY_SHIFT_RIGHT";
	code_names[WILLIS_KEY_CTRL_LEFT] =        "WILLIS_KEY_CTRL_LEFT";
	code_names[WILLIS_KEY_MOD_LEFT] =         "WILLIS_KEY_MOD_LEFT";
	code_names[WILLIS_KEY_ALT_LEFT] =         "WILLIS_KEY_ALT_LEFT";
	code_names[WILLIS_KEY_SPACE] =            "WILLIS_KEY_SPACE";
	code_names[WILLIS_KEY_ALT_RIGHT] =        "WILLIS_KEY_ALT_RIGHT";
	code_names[WILLIS_KEY_MOD_RIGHT] =        "WILLIS_KEY_MOD_RIGHT";
	code_names[WILLIS_KEY_MENU] =             "WILLIS_KEY_MENU";
	code_names[WILLIS_KEY_CTRL_RIGHT] =       "WILLIS_KEY_CTRL_RIGHT";
	code_names[WILLIS_KEY_PRINT_SCREEN] =     "WILLIS_KEY_PRINT_SCREEN";
#if 0
	code_names[WILLIS_KEY_SCROLL_LOCK] =      "WILLIS_KEY_SCROLL_LOCK";
	code_names[WILLIS_KEY_PAUSE] =            "WILLIS_KEY_PAUSE";
#endif
	code_names[WILLIS_KEY_INSERT] =           "WILLIS_KEY_INSERT";
	code_names[WILLIS_KEY_DELETE] =           "WILLIS_KEY_DELETE";
	code_names[WILLIS_KEY_HOME] =             "WILLIS_KEY_HOME";
	code_names[WILLIS_KEY_END] =              "WILLIS_KEY_END";
	code_names[WILLIS_KEY_PAGE_UP] =          "WILLIS_KEY_PAGE_UP";
	code_names[WILLIS_KEY_PAGE_DOWN] =        "WILLIS_KEY_PAGE_DOWN";
	code_names[WILLIS_KEY_UP] =               "WILLIS_KEY_UP";
	code_names[WILLIS_KEY_DOWN] =             "WILLIS_KEY_DOWN";
	code_names[WILLIS_KEY_LEFT] =             "WILLIS_KEY_LEFT";
	code_names[WILLIS_KEY_RIGHT] =            "WILLIS_KEY_RIGHT";
	code_names[WILLIS_KEY_NUM_LOCK] =         "WILLIS_KEY_NUM_LOCK";
	code_names[WILLIS_KEY_NUM_SLASH] =        "WILLIS_KEY_NUM_SLASH";
	code_names[WILLIS_KEY_NUM_ASTERISK] =     "WILLIS_KEY_NUM_ASTERISK";
	code_names[WILLIS_KEY_NUM_HYPHEN_MINUS] = "WILLIS_KEY_NUM_HYPHEN_MINUS";
	code_names[WILLIS_KEY_NUM_PLUS] =         "WILLIS_KEY_NUM_PLUS";
	code_names[WILLIS_KEY_NUM_ENTER] =        "WILLIS_KEY_NUM_ENTER";
	code_names[WILLIS_KEY_NUM_DELETE] =       "WILLIS_KEY_NUM_DELETE";
	code_names[WILLIS_KEY_NUM_0] =            "WILLIS_KEY_NUM_0";
	code_names[WILLIS_KEY_NUM_1] =            "WILLIS_KEY_NUM_1";
	code_names[WILLIS_KEY_NUM_2] =            "WILLIS_KEY_NUM_2";
	code_names[WILLIS_KEY_NUM_3] =            "WILLIS_KEY_NUM_3";
	code_names[WILLIS_KEY_NUM_4] =            "WILLIS_KEY_NUM_4";
	code_names[WILLIS_KEY_NUM_5] =            "WILLIS_KEY_NUM_5";
	code_names[WILLIS_KEY_NUM_6] =            "WILLIS_KEY_NUM_6";
	code_names[WILLIS_KEY_NUM_7] =            "WILLIS_KEY_NUM_7";
	code_names[WILLIS_KEY_NUM_8] =            "WILLIS_KEY_NUM_8";
	code_names[WILLIS_KEY_NUM_9] =            "WILLIS_KEY_NUM_9";

	char** state_names = context->event_state_names;
	code_names[WILLIS_STATE_NONE] =    "WILLIS_STATE_NONE";
	code_names[WILLIS_STATE_PRESS] =   "WILLIS_STATE_PRESS";
	code_names[WILLIS_STATE_RELEASE] = "WILLIS_STATE_RELEASE";

	// TODO init context variables

	return context;
}

void willis_start(
	struct willis* context,
	void* data,
	struct willis_error_info* error)
{
	context->backend_callbacks.start(context, data, error);
}

bool willis_handle_event(
	struct willis* context,
	void* event,
	struct willis_event_info* event_info,
	struct willis_error_info* error)
{
	return
		context->backend_callbacks.handle_event(
			context,
			event,
			event_info,
			error);
}

const char* willis_get_event_code_name(
	struct willis* context,
	enum willis_event_code event_code,
	struct willis_error_info* error)
{
	if (event_code < WILLIS_CODE_COUNT)
	{
		willis_error_ok(error);
		return context->event_code_names[event_code];
	}

	willis_error_throw(context, error, WILLIS_ERROR_EVENT_CODE_INVALID);
	return NULL;
}

const char* willis_get_event_state_name(
	struct willis* context,
	enum willis_event_state event_state,
	struct willis_error_info* error)
{
	if (event_state < WILLIS_STATE_COUNT)
	{
		willis_error_ok(error);
		return context->event_state_names[event_state];
	}

	willis_error_throw(context, error, WILLIS_ERROR_EVENT_STATE_INVALID);
	return NULL;
}

void willis_mouse_grab(
	struct willis* context,
	struct willis_error_info* error)
{
	context->backend_callbacks.mouse_grab(context, error);
}

void willis_mouse_ungrab(
	struct willis* context,
	struct willis_error_info* error)
{
	context->backend_callbacks.mouse_ungrab(context, error);
}

void willis_stop(
	struct willis* context,
	struct willis_error_info* error)
{
	context->backend_callbacks.stop(context, error);
}

void willis_clean(
	struct willis* context,
	struct willis_error_info* error)
{
	context->backend_callbacks.clean(context, error);
	free(context);
}
