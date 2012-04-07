#ifndef _SCM_H
#define _SCM_H

#include "scmmodel/scm.h"

/* 
 * Prototypes
 */

int scm_init(scm_t **);
void scm_fini(scm_t *);
void *scm_memcpy(scm_t *scm, void *dst, const void *src, size_t n);

void scm_stat_reset(scm_t *scm);
void scm_stat_print(scm_t *scm);


#endif /* _SCM_H */
