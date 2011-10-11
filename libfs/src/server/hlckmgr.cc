#include "server/hlckmgr.h"

// When a lock is downgraded we check that any lock acquired under the downgraded 
// lock abides the hierarchy rule. Usually there will not be many such locks 
