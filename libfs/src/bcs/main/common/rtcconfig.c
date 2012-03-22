#include "bcs/main/common/rtcconfig.h"
#include <libconfig.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#define ENVVAR_MAX_LEN 128

static inline int 
env_setting_lookup(const char *name, char **value_str)
{
	char *val;
	char normalized_name[ENVVAR_MAX_LEN];
	int  i;

	if ((strlen(name)) > ENVVAR_MAX_LEN) {
		return CONFIG_FALSE;
	}
	
	for (i=0; name[i]; i++) {
		if (name[i] == '.') {
			normalized_name[i] = '_';
		} else {
			normalized_name[i] = toupper(name[i]);
		}
	}
	normalized_name[i] = '\0';
	
	val = getenv(normalized_name);
	if (val) {
		*value_str = val;
		return CONFIG_TRUE;
	} else {
		return CONFIG_FALSE;
	}
}


static inline int
env_setting_lookup_int(const char *name, int *value)
{
	char *value_str;

	if (env_setting_lookup(name, &value_str) == CONFIG_FALSE) {
		return CONFIG_FALSE;
	}

	if (value_str) {
		*value = atoi(value_str);
		return CONFIG_TRUE;
	} else {
		return CONFIG_FALSE;
	}
}


static inline int
env_setting_lookup_bool(const char *name, int *value)
{
	return env_setting_lookup_int(name, value);
}


static inline int 
env_setting_lookup_string(const char *name, char **value)
{
	return env_setting_lookup(name, value);
}


int
__cconfig_lookup_bool(config_t *cfg, const char *name, int *value) 
{
	int val;
	int found_val = 0;

	if (env_setting_lookup_bool(name, &val) == CONFIG_TRUE) {
		found_val = 1;
	} else {
	    if (config_lookup_bool(cfg, name, &val) == CONFIG_TRUE) {
			found_val = 1;
		}
	}

	if (found_val)	{
		*value = val;
		return CONFIG_TRUE;
	}
	return CONFIG_FALSE;
}


int
__cconfig_lookup_valid_bool(config_t *cfg, 
                     const char *name, 
                     int *value, 
                     int validity_check, ...)
{
	return __cconfig_lookup_bool(cfg, name, value);
}


int
__cconfig_lookup_int(config_t *cfg, const char *name, int *value)
{
	int val;
	int found_val = 0;

	if (env_setting_lookup_int(name, &val) == CONFIG_TRUE) {
		found_val = 1;
	} else {
	    if (config_lookup_int(cfg, name, &val) == CONFIG_TRUE) {
			found_val = 1;
		}
	}

	if (found_val)	{
		*value = val;
		return CONFIG_TRUE;
	}
	return CONFIG_FALSE;
}


int
__cconfig_lookup_valid_int(config_t *cfg, 
                           const char *name, 
                           int *value, 
                           int validity_check, ...)
{
	config_setting_t *group;
	int              min;
	int              max;
	int              list_length;
	int              i;
	int              val;
	int              listval;
	va_list          ap;
	int              found_val  = 0;

	if (__cconfig_lookup_int(cfg, name, &val) == CONFIG_TRUE) {
		switch (validity_check) {
			case CONFIG_NO_CHECK:
				*value = val;
				return CONFIG_TRUE;
			case CONFIG_RANGE_CHECK:
				va_start(ap, validity_check);
				min = va_arg(ap, int);
				max = va_arg(ap, int);
				va_end(ap);
				if (*value >= min && *value <= max) {
					*value = val;
					return CONFIG_TRUE;
				}
				break;
			case CONFIG_LIST_CHECK:
				va_start(ap, validity_check);
				list_length = va_arg(ap, int);
				for (i=0; i<list_length; i++) {
					listval = va_arg(ap, int);
					if (val == listval) {
						*value = val;
						return CONFIG_TRUE;
					}
				}
				va_end(ap);
				break;
		}
	}
	return CONFIG_FALSE;
}


int
__cconfig_lookup_string(config_t *cfg, const char *name, char **value)
{
	char *val;
	int  found_val = 0;

	if (env_setting_lookup_string(name, &val) == CONFIG_TRUE) {
		found_val = 1;
	} else {	
	    if (config_lookup_string(cfg, name, (const char**) &val) == CONFIG_TRUE) {
			found_val = 1;
		}
	}

	if (found_val)	{
		*value = val;
		return CONFIG_TRUE;
	}
	return CONFIG_FALSE;
}


int
__cconfig_lookup_valid_string(config_t *cfg, 
                              const char *name, 
                              char **value, 
                              int validity_check, ...)
{
	int       list_length;
	int       i;
	char      *val;
	va_list   ap;
	int       found_val = 0;

	if (__cconfig_lookup_string(cfg, name, &val) == CONFIG_TRUE) {
		switch (validity_check) {
			case CONFIG_NO_CHECK:
				*value = val;
				return CONFIG_TRUE;
			case CONFIG_RANGE_CHECK:
				break;
			case CONFIG_LIST_CHECK:
				va_start(ap, validity_check);
				list_length = va_arg(ap, int);
				for (i=0; i<list_length; i++) {
					if (strcmp(val, va_arg(ap, char *))==0) {
						*value = val;
						return CONFIG_TRUE;
					}
				}
				va_end(ap);
				break;
		}
	}
	return CONFIG_FALSE;
}


int 
__cconfig_init(config_t *cfg, const char *config_file)
{
	char  buf[128];
	char* env_config_file;

	if (env_config_file = getenv("LIBFS_CONFIG")) {
		config_file = env_config_file;
	}
	
	config_init(cfg);
	return config_read_file(cfg, config_file);
}
