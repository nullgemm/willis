#ifndef H_WILLIS_INTERNAL_NIX
#define H_WILLIS_INTERNAL_NIX

#include "willis.h"

#include <stdint.h>
#include <xkbcommon/xkbcommon.h>

struct willis_xkb
{
	struct xkb_context* context;
	struct xkb_keymap* keymap;
	struct xkb_state* state;
	const char* locale;
	struct xkb_compose_table* compose_table;
	struct xkb_compose_state* compose_state;
};

void willis_xkb_init_locale(
	struct willis_xkb* xkb_common);

void willis_xkb_init_compose(
	struct willis_xkb* xkb_common);

enum willis_event_code willis_xkb_translate_keycode(
	uint8_t keycode);

void willis_xkb_utf8_simple(
	struct willis* context,
	struct willis_xkb* xkb_common,
	xkb_keycode_t keycode,
	char** utf8_string,
	size_t* utf8_size,
	struct willis_error_info* error);

void willis_xkb_utf8_compose(
	struct willis* context,
	struct willis_xkb* xkb_common,
	xkb_keycode_t keycode,
	char** utf8_string,
	size_t* utf8_size,
	struct willis_error_info* error);

#endif
