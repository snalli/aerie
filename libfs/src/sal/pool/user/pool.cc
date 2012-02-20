/**
 * \brief A user-mode implementation of the pool abstraction.
 *
 * The pool is implemented on top of a persistent region. 
 */

#include "sal/pool/user/pool_i.h"

int
Pool::Create(const char* pathname, size_t size)
{
	

	// persistent region must accomodate the pool's raw space and the pool's
	// metadata:
	// RAW SPACE: size
	// METADATA : sizeof(bitset to track each page of RAW space)



}


int
Pool::Open(const char* pathname, Pool** pool)
{
	

}
