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

	struct willis_event_info* event_info;
};

#endif
