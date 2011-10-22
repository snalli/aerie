#include "common/debug.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include "common/config.h"


int         dbg_modules[dbg_module_count];
int         dbg_level = 0;
char*       dbg_identifier = "";
static char __dbg_identifier[128];

static config_t dbg_cfg;

void dbg_set_level(int level)
{
	dbg_level = level;
}


void dbg_set_identifier(char* identifier)
{
	dbg_identifier = identifier;
}


static int strrep(char *target, char *source, char oldc, char newc)
{
	int i;

	for (i=0; source[i]; i++) {
		if (source[i] == oldc) {
			target[i] = newc;
		} else {
			target[i] = source[i];
		}
	}
	target[i] = '\0';
	return 0;
}


int dbg_init(int level, char* identifier)
{
	my_config_init(&dbg_cfg, "libfs.ini");

	// if user hasn't provided a debugging level then get it from the 
	// configuration env/file
	if (level<0) {
		my_config_lookup_int(&dbg_cfg, "debug_level", &dbg_level);
	} else {
		dbg_level = level;
	}

	// if user hasn't provide an identifier then check whether the environment 
	// provides one, othewise create one based on process' pid 
	if (!identifier) {
		dbg_identifier = getenv("DEBUG_IDENTIFIER");
		if (!dbg_identifier) {
			sprintf(__dbg_identifier, "%d", getpid()); 
			dbg_identifier = __dbg_identifier;
		}
	} else {
		dbg_identifier = identifier;
	}

	// read per module debugging flags
#define STR(name) #name
#define ACTION(name)                                                           \
	do {                                                                       \
		char dotstr[128];                                                      \
		strrep(dotstr, STR(debug_module_##name), '_', '.');                    \
		my_config_lookup_bool(&dbg_cfg, dotstr,                                \
		                      &dbg_modules[dbg_module_##name]);                \
	} while (0);

	FOREACH_DEBUG_MODULE(ACTION)
#undef ACTION
}


void
dbg_backtrace (void)
{
	void *array[10];
	size_t size;
	char **strings;
	size_t i;
 
	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);
									      
	printf ("Obtained %zd stack frames.\n", size);
																	      
	for (i = 0; i < size; i++)
		printf ("%s\n", strings[i]);
	free (strings);
}
