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


inline int RoundUpSize(size_t size, size_t multiple_size)
{
	return ((size / multiple_size) + 1) * multiple_size;
}


static inline size_t
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

/**
 * ASSERT_OK(expr) — always evaluates expr (unlike assert which disappears in
 * Release/NDEBUG builds).  In Debug builds the return value is checked == 0;
 * in Release builds the call still happens but the check is skipped.
 *
 * Use instead of assert(f() == 0) whenever the expression has side-effects
 * that must execute in production (e.g. Lock(), Link(), Load()).
 */
#include <assert.h>
#include <stdint.h>
#ifndef ASSERT_OK
#define ASSERT_OK(expr) do { intptr_t _aok_r = (intptr_t)(expr); assert(_aok_r == 0); (void)_aok_r; } while (0)
#endif

#endif // __STAMNOS_COMMON_UTIL_H
