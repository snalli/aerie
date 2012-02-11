#ifndef _REGISTRY_H_AGT127
#define _REGISTRY_H_AGT127

#include <sys/types.h>
#include "rpc/rpc.h"

namespace client {

class Ipc; // forward declaration

class Registry {
public:
	Registry(Ipc* ipc_layer);
	int Lookup(const char *name, uint64_t* val);
	int Add(const char *name, uint64_t val);
	int Remove(const char *name);

private:
	Ipc* ipc_;
};

} // namespace client

#endif
