#ifndef _NAMESERVICE_H_AGT127
#define _NAMESERVICE_H_AGT127

#include <sys/types.h>
#include "rpc/rpc.h"

typedef void inode_t;

class NameService {
public:
	NameService(rpcc *c, unsigned int);
	int Lookup(const char *name, inode_t **inode);
	int Link(const char *name, inode_t *inode);
	int Remove(const char *name);

private:
	rpcc*        client_;
	unsigned int principal_id_;

};


#endif
