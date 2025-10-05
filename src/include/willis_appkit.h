#ifndef H_WILLIS_APPKIT
#define H_WILLIS_APPKIT

#include "willis.h"

struct willis_appkit_data
{
	void* data;
};

#if !defined(WILLIS_SHARED)
void willis_prepare_init_appkit(
	struct willis_config_backend* config);
#endif

#endif
