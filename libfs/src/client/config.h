#ifndef _CLIENT_CONFIG_H_WER192
#define _CLIENT_CONFIG_H_WER192

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

#endif /* _CLIENT_CONFIG_H_WER192 */
