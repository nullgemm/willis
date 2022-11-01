#include "include/willis.h"
#include "common/willis_private.h"
#include "common/willis_error.h"

#include <stdbool.h>

#if defined(WILLIS_ERROR_LOG_MANUAL) || defined(WILLIS_ERROR_LOG_THROW)
	#include <stdio.h>
#endif

#ifdef WILLIS_ERROR_ABORT
	#include <stdlib.h>
#endif

void willis_error_init(
	struct willis* context)
{
#ifndef WILLIS_ERROR_SKIP
	char** log = context->error_messages;

	log[WILLIS_ERROR_OK] =
		"out-of-bound error message";
	log[WILLIS_ERROR_NULL] =
		"null pointer";
	log[WILLIS_ERROR_ALLOC] =
		"failed malloc";
	log[WILLIS_ERROR_BOUNDS] =
		"out-of-bounds index";
	log[WILLIS_ERROR_DOMAIN] =
		"invalid domain";
	log[WILLIS_ERROR_FD] =
		"invalid file descriptor";

	log[WILLIS_ERROR_EVENT_CODE_INVALID] =
		"invalid event code";
	log[WILLIS_ERROR_EVENT_STATE_INVALID] =
		"invalid event state";
#endif
}

void willis_error_log(
	struct willis* context,
	struct willis_error_info* error)
{
#ifndef WILLIS_ERROR_SKIP
	#ifdef WILLIS_ERROR_LOG_MANUAL
		#ifdef WILLIS_ERROR_LOG_DEBUG
			fprintf(
				stderr,
				"error in %s line %u: ",
				error->file,
				error->line);
		#endif

		if (error->code < WILLIS_ERROR_COUNT)
		{
			fprintf(stderr, "%s\n", context->error_messages[error->code]);
		}
		else
		{
			fprintf(stderr, "%s\n", context->error_messages[0]);
		}
	#endif
#endif
}

const char* willis_error_get_msg(
	struct willis* context,
	struct willis_error_info* error)
{
	if (error->code < WILLIS_ERROR_COUNT)
	{
		return context->error_messages[error->code];
	}
	else
	{
		return context->error_messages[0];
	}
}

enum willis_error willis_error_get_code(
	struct willis_error_info* error)
{
	return error->code;
}

const char* willis_error_get_file(
	struct willis_error_info* error)
{
	return error->file;
}

unsigned willis_error_get_line(
	struct willis_error_info* error)
{
	return error->line;
}

void willis_error_ok(
	struct willis_error_info* error)
{
	error->code = WILLIS_ERROR_OK;
	error->file = "";
	error->line = 0;
}

void willis_error_throw_extra(
	struct willis* context,
	struct willis_error_info* error,
	enum willis_error code,
	const char* file,
	unsigned line)
{
#ifndef WILLIS_ERROR_SKIP
	error->code = code;
	error->file = file;
	error->line = line;

	#ifdef WILLIS_ERROR_LOG_THROW
		#ifdef WILLIS_ERROR_LOG_DEBUG
			fprintf(
				stderr,
				"error in %s line %u: ",
				file,
				line);
		#endif

		if (error->code < WILLIS_ERROR_COUNT)
		{
			fprintf(stderr, "%s\n", context->error_messages[error->code]);
		}
		else
		{
			fprintf(stderr, "%s\n", context->error_messages[0]);
		}
	#endif

	#ifdef WILLIS_ERROR_ABORT
		abort();
	#endif
#endif
}
