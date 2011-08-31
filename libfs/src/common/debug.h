#ifndef _DEBUG_H_JKL896
#define _DEBUG_H_JKL896

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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


extern int dbg_level;


#define dbg_log(level, format, ...)                                            \
  do {                                                                         \
    if (level && (level <= dbg_level ||                                        \
                  level <= dbg_terminate_level)) {                             \
      fprintf(stderr, "%s in %s <%s,%d>: " format, dbg_code2str[level],        \
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



void dbg_set_level(int);


#ifdef __cplusplus
}
#endif

#endif
