#ifndef __STAMNOS_FS_CLIENT_CONFIG_H
#define __STAMNOS_FS_CLIENT_CONFIG_H

#include "common/config.h"

namespace client {

class Config {
public:	
	static int Init();

	static config_t cfg_;
	class Var {
	public:
		static int      debug_level_;
	};

};

}

#endif /* __STAMNOS_FS_CLIENT_CONFIG_H */
