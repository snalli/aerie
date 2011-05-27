#ifndef _EXTENTPOOL_H_AGT127
#define _EXTENTPOOL_H_AGT127

#include <sys/types.h>
#include "rpc/rpc.h"
#include "common/vistaheap.h"
#include "common/pheap.h"


class ExtentPool {
public:
	ExtentPool(rpcc *c, unsigned int);
	unsigned long long CreateExtent(size_t size);
	int DeleteExtent(unsigned long long extent_id, size_t size);
	void *AccessExtent(unsigned long long extent_id, size_t size);

private:
	rpcc*        client_;
	int          fd_;
	unsigned int principal_id_;

};


#endif
