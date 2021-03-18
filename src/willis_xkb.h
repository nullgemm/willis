#ifndef H_WILLIS_INTERNAL_XKB
#define H_WILLIS_INTERNAL_XKB

#include <xkbcommon/xkbcommon.h>

struct willis_xkb
{
	struct xkb_context* xkb_ctx;
	struct xkb_keymap* xkb_keymap;
	struct xkb_state* xkb_state;

	const char* xkb_locale;
	struct xkb_compose_table* xkb_compose_table;
	struct xkb_compose_state* xkb_compose_state;
};

#endif
