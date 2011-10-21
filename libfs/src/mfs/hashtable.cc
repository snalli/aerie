#include "mfs/hashtable.h"
#include <stdint.h>
#include <string.h>
#include "client/backend.h"
#include "common/errno.h"
#include "common/hash.h"


const int HT_REHASH = 1;

inline uint64_t lgl2lnr (uint64_t logical_addr)
{
	return logical_addr;
}



