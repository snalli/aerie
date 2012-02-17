#ifndef __STAMNOS_DPO_CLIENT_REGISTRY_H
#define __STAMNOS_DPO_CLIENT_REGISTRY_H

#include <sys/types.h>
#include "dpo/main/common/obj.h"
#include "ipc/main/client/ipc-opaque.h"


namespace dpo {
namespace client {

class Registry {
public:
	Registry(::client::Ipc* ipc);
	int Lookup(const char *name, dpo::common::ObjectId* oid);
	int Add(const char *name, dpo::common::ObjectId oid);
	int Remove(const char *name);

private:
	::client::Ipc* ipc_;
};

} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_REGISTRY_H
