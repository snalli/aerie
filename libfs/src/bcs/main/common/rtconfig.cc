#include "bcs/main/common/rtconfig.h"
#include <stdlib.h>
#include <stdio.h>


config_t Config::cfg_;


int 
Config::Init()
{
	return __cconfig_init(&cfg_, "libfs.ini");
}
