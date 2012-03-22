#ifndef __STAMNOS_BCS_COMMON_CONFIG_H
#define __STAMNOS_BCS_COMMON_CONFIG_H

#include "bcs/main/common/rtcconfig.h"

class Config {
public:	
	static int Init();
	static config_t* cfg() { return &cfg_; }
	static int Lookup(const char* name, bool* val) {
		return __cconfig_lookup_bool(&cfg_, name, (int*)val);
	}
	static int Lookup(const char* name, int* val) {
		return __cconfig_lookup_int(&cfg_, name, val);
	}
	static int Lookup(const char* name, char** val) {
		return __cconfig_lookup_string(&cfg_, name, val);
	}
private:
	static config_t cfg_;
};

#endif /* __STAMNOS_BCS_COMMON_CONFIG_H */
