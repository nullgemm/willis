#include "willis.h"

char* willis_get_utf8_string(struct willis* willis)
{
	return willis->utf8_string;
}

size_t willis_get_utf8_size(struct willis* willis)
{
	return willis->utf8_size;
}

bool willis_get_mouse_grab(struct willis* willis)
{
	return willis->mouse_grab;
}

int16_t willis_get_mouse_x(struct willis* willis)
{
	return willis->mouse_x;
}

int16_t willis_get_mouse_y(struct willis* willis)
{
	return willis->mouse_y;
}

int64_t willis_get_diff_x(struct willis* willis)
{
	return willis->diff_x;
}

int64_t willis_get_diff_y(struct willis* willis)
{
	return willis->diff_y;
}
