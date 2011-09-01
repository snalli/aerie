#ifndef _CONFIG_GENERIC_H_DFA617
#define _CONFIG_GENERIC_H_DFA617

#include <stdio.h>
#include <libconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Make sure we don't redefine a macro already defined in libconfig.h */

#ifdef CONFIG_NO_CHECK
# error "ERROR: Redefining previously defined CONFIG_NO_CHECK"
#else
# define CONFIG_NO_CHECK    0
#endif

#ifdef CONFIG_RANGE_CHECK
# error "ERROR: Redefining previously defined CONFIG_RANGE_CHECK"
#else
# define CONFIG_RANGE_CHECK 1
#endif

#ifdef CONFIG_LIST_CHECK
# error "ERROR: Redefining previously defined CONFIG_LIST_CHECK"
#else
# define CONFIG_LIST_CHECK  2
#endif


/** 
 * The lookup functions return the value of a configuration variable based on 
 * the following order: 
 *  1) value of environment variable
 *  2) value in configuration file variable
 *  
 * If the variable is not found then a lookup function does not set the value.
 */

int my_config_lookup_bool(config_t *cfg, char *name, int *value);
int my_config_lookup_int(config_t *cfg, char *name, int *value);
int my_config_lookup_string(config_t *cfg, char *name, char **value);
int my_config_lookup_valid_bool(config_t *cfg, char *name, int *value, int validity_check, ...);
int my_config_lookup_valid_int(config_t *cfg, char *name, int *value, int validity_check, ...);
int my_config_lookup_valid_string(config_t *cfg, char *name, char **value, int validity_check, ...);
int my_config_init(config_t *cfg, char *config_file);

#ifdef __cplusplus
}
#endif


#endif /* _CONFIG_GENERIC_H_DFA617 */
