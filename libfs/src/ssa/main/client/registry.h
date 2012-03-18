#ifndef __STAMNOS_SSA_CLIENT_REGISTRY_H
#define __STAMNOS_SSA_CLIENT_REGISTRY_H

#include <sys/types.h>
#include "ssa/main/common/obj.h"
#include "bcs/bcs-opaque.h"


namespace ssa {
namespace client {

class Registry {
public:
	Registry(::client::Ipc* ipc);
	int Lookup(const char *name, ssa::common::ObjectId* oid);
	int Add(const char *name, ssa::common::ObjectId oid);
	int Remove(const char *name);

private:
	::client::Ipc* ipc_;
};

} // namespace client
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_REGISTRY_H
