#ifndef _REGISTRY_H_AGT127
#define _REGISTRY_H_AGT127

#include <sys/types.h>
#include "rpc/rpc.h"


class Registry {
public:
	Registry(rpcc *c, unsigned int);
	int Lookup(const char *name, void**);
	int Add(const char *name, void*);
	int Remove(const char *name);

private:
	rpcc*        client_;
	unsigned int principal_id_;

};


#endif
