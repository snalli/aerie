#include "client/config.h"
#include <stdlib.h>
#include <stdio.h>


namespace client {

config_t Config::cfg_;
int      Config::Var::debug_level_ = 0;



int 
Config::Init()
{
	my_config_init(&cfg_, "libfs.ini");

	return 0;
}


}
