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
