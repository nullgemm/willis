#ifndef H_WILLIS_INTERNAL_APPKIT_HELPERS
#define H_WILLIS_INTERNAL_APPKIT_HELPERS

#include "include/willis.h"
#include "appkit/macos.h"

#import <AppKit/AppKit.h>

enum willis_event_code appkit_helpers_keycode_table(
	uint8_t code);

void appkit_helpers_send_system_keys(
	struct appkit_backend* backend,
	struct willis_event_info* event_info,
	NSEventModifierFlags flags,
	NSEventModifierFlags comparison,
	uint8_t code);

#endif
