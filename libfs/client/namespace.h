#ifndef _NAMESPACE_H_AGT127
#define _NAMESPACE_H_AGT127

#include <sys/types.h>
#include "rpc/rpc.h"

class NameSpace {
public:
	NameSpace(rpcc *c, unsigned int, const char*);
	int Lookup(const char *name, void**);
	int Link(const char *name, void*);
	int Unlink(const char *name);

private:
	rpcc*        client_;
	unsigned int principal_id_;
	char         namespace_name_[128];

};


#endif
