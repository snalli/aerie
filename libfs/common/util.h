#ifndef _UTIL_H_AKL112
#define _UTIL_H_AKL112

#include <sys/types.h>

const int kPageSize = 4096;

inline int num_pages(size_t size)
{
	return ((size % kPageSize) == 0 ? 0 : 1) + size/kPageSize;
}

inline int 
str_is_dot(char* str) 
{
	if (str[1] == '\0') {
		if (str[0] == '.') {
			return 1;
		} 
	} else if (str[2] == '\0') {
		if (str[0] == '.' && str[1] == '.') {
			return 2;
		} 
	}

	return 0;
}


#ifdef __cplusplus 
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif
