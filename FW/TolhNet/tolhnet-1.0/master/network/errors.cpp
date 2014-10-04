//=========================================================================
// errors.cpp
// List of error codes used by the network-layer code
//=========================================================================

#include "errors.h"

#define LIST(x) #x,
static const char *error_names[] = { ERROR_CODES(LIST) };
#undef LIST

static const char hex[] = "0123456789ABCDEF";

std::string error_name (int x)
{
	if (x > 0) x = 0;
	x = -x;
	if ((unsigned) x < sizeof error_names / sizeof *error_names)
		return error_names[x];
	if (x < 0x10000) {
		char error[] = {'E', hex[x >> 12], hex[(x >> 8) & 0x0F], hex[(x >> 4) & 0x0F], hex[x & 0x0F], 0};
		return error;
	}
	return "E????";
}
