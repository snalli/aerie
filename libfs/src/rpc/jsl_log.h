#ifndef __JSL_LOG_H__
#define __JSL_LOG_H__ 1

#include "common/debug.h"

// map JSL debugging levels to the generic DBG ones
enum dbcode {
	JSL_DBG_OFF = 0,
	JSL_DBG_1 = DBG_CRITICAL, // Critical
	JSL_DBG_2 = DBG_INFO, // Error
	JSL_DBG_3 = DBG_INFO, // Info
	JSL_DBG_4 = DBG_DEBUG, // Debugging
};

#if 0
#define jsl_log(level, format, ...)                                            \
  do {                                                                         \
    if (level && (level <= dbg_level)) {                                         \
      fprintf(stderr, "[%s] %s in %s <%s,%d>: " format,                        \
              dbg_identifier,                                                  \
              dbg_code2str[level],                                             \
              __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__);                \
    }			                                                               \
  } while(0);
#endif

#define jsl_log(level, format, ...)                                            \
	DBG_LOG(level, dbg_module_rpc, format, ##__VA_ARGS__)

#endif // __JSL_LOG_H__
