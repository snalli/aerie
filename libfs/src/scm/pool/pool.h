#ifndef __STAMNOS_SCM_POOL_H
#define __STAMNOS_SCM_POOL_H
	
#ifdef _SCM_POOL_KERNELMODE
#include "scm/pool/kernel/pool.h"
#endif

#ifdef _SCM_POOL_USERMODE
#include "scm/pool/user/pool.h"
#endif

#endif // __STAMNOS_SCM_POOL_H
