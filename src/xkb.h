#ifndef H_WILLIS_XKB
#define H_WILLIS_XKB

#include "willis.h"

#include <stdint.h>
#include <xkbcommon/xkbcommon.h>

enum willis_event_code willis_translate_keycode_x11(uint8_t keycode);
void willis_xkb_init_locale(struct willis* willis);
void willis_xkb_init_compose(struct willis* willis);
void willis_utf8_simple(struct willis* willis, xkb_keycode_t keycode);
void willis_utf8_compose(struct willis* willis, xkb_keycode_t keycode);

#endif
