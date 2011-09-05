#ifndef _DEBUG_H_JKL896
#define _DEBUG_H_JKL896

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


#define FOREACH_DEBUG_MODULE(ACTION)                        \
	ACTION(all) /* special name that covers all modules */  \
	ACTION(rpc)                                             \
	ACTION(client_lckmgr)  



#define ACTION(name) \
	dbg_module_##name,
enum {
	FOREACH_DEBUG_MODULE(ACTION)
	dbg_module_count
};
#undef ACTION



enum dbg_code {
	DBG_OFF = 0,
	DBG_CRITICAL = 1, // Critical
	DBG_ERROR    = 2, // Error
	DBG_WARNING  = 3, // Warning
	DBG_INFO     = 4, // Info
	DBG_DEBUG    = 5, // Debugging
};

static char* dbg_code2str[] = {
	(char*) "Off",
	(char*) "CRITICAL",
	(char*) "ERROR",
	(char*) "WARNING",
	(char*) "INFO",
	(char*) "DEBUG",
};

const int dbg_terminate_level = DBG_ERROR;
const int dbg_stderr_level = DBG_WARNING;

extern int   dbg_modules[];
extern int   dbg_level;
extern char* dbg_identifier;

#define DBG_MODULE(name) dbg_module_##name

#define dbg_log(level, format, ...)                                            \
  do {                                                                         \
    FILE* ferr = stdout;                                                       \
    if (level && (level <= dbg_level ||                                        \
                  level <= dbg_terminate_level))                               \
    {                                                                          \
      if (level <= dbg_stderr_level) {                                         \
        ferr=stderr;                                                           \
      }                                                                        \
      fprintf(ferr, "[%s] %s in %s <%s,%d>: " format,                          \
              dbg_identifier,                                                  \
              dbg_code2str[level],                                             \
              __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__);                \
      if (level <= dbg_terminate_level) {                                      \
        exit(-1);                                                              \
      }	                                                                       \
    }			                                                               \
  } while(0);


#define DBG_LOG(level, module, format, ...)                                    \
  do {                                                                         \
    FILE* ferr = stdout;                                                       \
    if (level &&                                                               \
	    (dbg_modules[module] || dbg_modules[dbg_module_all]) &&                \
	    (level <= dbg_level || level <= dbg_terminate_level))                  \
    {                                                                          \
      if (level <= dbg_stderr_level) {                                         \
        ferr=stderr;                                                           \
      }                                                                        \
 	  fprintf(ferr, "[%s] %s in %s <%s,%d>: " format,                          \
              dbg_identifier,                                                  \
              dbg_code2str[level],                                             \
              __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__);                \
      if (level <= dbg_terminate_level) {                                      \
        exit(-1);                                                              \
      }	                                                                       \
    }			                                                               \
  } while(0);



#define VERIFY(condition)                                                      \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "Assumption \"%s\"\nFailed in file %s: at line:%i\n",    \
              #condition,__FILE__,__LINE__);                                   \
      dbg_log (DBG_CRITICAL, #condition);}                                     \
  } while (0);


inline void Assert(int assertion, char* error) 
{
	if(!assertion) {
		printf("Assertion Failed: %s\n",error);
		dbg_log (DBG_CRITICAL, "Exiting From Function Assert(...)\n");
	}
}


int dbg_init(int level, char* identifier);


#ifdef __cplusplus
}
#endif

#endif
