#ifndef __STAMNOS_COMMON_UTIL_H
#define __STAMNOS_COMMON_UTIL_H

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

static const char  __whitespaces[] = "                                                              ";
#define WHITESPACE(len) &__whitespaces[sizeof(__whitespaces) - (len) -1]


const int kPageSize = 4096;

inline int num_pages(size_t size)
{
	return ((size % kPageSize) == 0 ? 0 : 1) + size/kPageSize;
}

inline int NumOfBlocks(size_t size, size_t block_size)
{
	return ((size % block_size) == 0 ? 0 : 1) + size/block_size;
}


static size_t
StringToSize(const char* cstr)
{
	size_t factor = 1;
	size_t size;
	int    last = strlen(cstr) - 1;

	switch (cstr[last]) {
		case 'K': case 'k':
			factor = 1024LLU;
			break;
		case 'M': case 'm':
			factor = 1024LLU*1024LLU;
			break;
		case 'G': case 'g':
			factor = 1024LLU*1024LLU*1024LLU;
			break;
	}
	size = factor * atoll(cstr);
	return size;
}


inline int 
str_is_dot(const char* str) 
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

#endif // __STAMNOS_COMMON_UTIL_H
