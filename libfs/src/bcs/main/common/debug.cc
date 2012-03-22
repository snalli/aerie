#include "bcs/main/common/debug.h"
#include "bcs/main/common/rtconfig.h"

int
Debug::Init(int level, const char* identifier)
{
	return dbg_init(Config::cfg(), level, identifier);
}
