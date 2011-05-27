#ifndef _UTIL_H_AKL112
#define _UTIL_H_AKL112

#include <sys/types.h>

const int kPageSize = 4096;

inline int num_pages(size_t size)
{
	return ((size % kPageSize) == 0 ? 0 : 1) + size/kPageSize;
}


#ifdef __cplusplus 
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
