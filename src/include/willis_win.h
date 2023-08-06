#ifndef H_WILLIS_WIN
#define H_WILLIS_WIN

#include "willis.h"
#include <windows.h>

struct willis_win_data
{
	void* data;
};

void willis_prepare_init_win(
	struct willis_config_backend* config);

#endif
