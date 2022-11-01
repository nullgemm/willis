#ifndef H_WILLIS_PRIVATE
#define H_WILLIS_PRIVATE

#include "include/willis.h"
#include "common/willis_error.h"

#include <stddef.h>

struct willis
{
	char* error_messages[WILLIS_ERROR_COUNT];
	void* backend_data;
	struct willis_config_backend backend_callbacks;

	char* event_code_names[WILLIS_CODE_COUNT];
	char* willis_event_state_names[WILLIS_STATE_COUNT];
};

#endif
