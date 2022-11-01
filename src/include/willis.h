#ifndef H_WILLIS
#define H_WILLIS

#include <stdbool.h>

struct willis;

enum willis_error
{
	WILLIS_ERROR_OK = 0,
	WILLIS_ERROR_NULL,
	WILLIS_ERROR_ALLOC,
	WILLIS_ERROR_BOUNDS,
	WILLIS_ERROR_DOMAIN,
	WILLIS_ERROR_FD,

	WILLIS_ERROR_COUNT,
};

struct willis_error_info
{
	enum willis_error code;
	const char* file;
	unsigned line;
};

struct willis_event_info
{
	// TODO fill
};

struct willis_config_backend
{
	void* data;

	void (*init)(
		struct willis* context,
		struct willis_error_info* error);

	void (*start)(
		struct willis* context,
		void* data,
		struct willis_error_info* error);

	bool (*handle_event)(
		struct willis* context,
		void* event,
		struct willis_event_info* event_info,
		struct willis_error_info* error);

	void (*stop)(
		struct willis* context,
		struct willis_error_info* error);

	void (*clean)(
		struct willis* context,
		struct willis_error_info* error);
};

struct willis* willis_init(
	struct willis_config_backend* config,
	struct willis_error_info* error);

void willis_start(
	struct willis* context,
	void* data,
	struct willis_error_info* error);

bool willis_handle_event(
	struct willis* context,
	void* event,
	struct willis_event_info* event_info,
	struct willis_error_info* error);

void willis_stop(
	struct willis* context,
	struct willis_error_info* error);

void willis_clean(
	struct willis* context,
	struct willis_error_info* error);

void willis_error_log(
	struct willis* context,
	struct willis_error_info* error);

const char* willis_error_get_msg(
	struct willis* context,
	struct willis_error_info* error);

enum willis_error willis_error_get_code(
	struct willis_error_info* error);

const char* willis_error_get_file(
	struct willis_error_info* error);

unsigned willis_error_get_line(
	struct willis_error_info* error);

void willis_error_ok(
	struct willis_error_info* error);

#endif
