#ifndef H_WILLIS_ERROR
#define H_WILLIS_ERROR

#include "include/willis.h"

#define willis_error_throw(context, error, code) \
	willis_error_throw_extra(\
		context,\
		error,\
		code,\
		WILLIS_ERROR_FILE,\
		WILLIS_ERROR_LINE)
#define WILLIS_ERROR_FILE __FILE__
#define WILLIS_ERROR_LINE __LINE__

void willis_error_throw_extra(
	struct willis* context,
	struct willis_error_info* error,
	enum willis_error code,
	const char* file,
	unsigned line);

void willis_error_init(
	struct willis* context);

#endif
