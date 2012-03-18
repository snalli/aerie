#include "ssa/main/client/registry.h"
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include "common/util.h"
#include "common/hrtime.h"
#include "bcs/bcs.h"
#include "ssa/main/common/obj.h"
#include "ssa/main/common/registry_protocol.h"

namespace ssa {
namespace client {

Registry::Registry(::client::Ipc* ipc)
	: ipc_(ipc)
{ }

int
Registry::Lookup(const char *name, ssa::common::ObjectId* oid)
{
	std::string           name_str;
	ssa::common::ObjectId oid_tmp;
	int                   intret;

	name_str = std::string(name);

	intret = ipc_->call(ssa::RegistryProtocol::kLookup, ipc_->id(), name_str, oid_tmp);
	if (intret) {
		return -intret;
	}
	*oid = oid_tmp;
	
	return 0;
}


int
Registry::Add(const char *name, ssa::common::ObjectId oid)
{
	std::string        name_str;
	int                r;
	int                intret;

	name_str = std::string(name);

	intret = ipc_->call(ssa::RegistryProtocol::kAdd, ipc_->id(), name_str, oid, r);
	if (intret) {
		return -intret;
	}

	return 0;
}


int
Registry::Remove(const char *name)
{
	std::string name_str;
	int         r;
	int         intret;

	name_str = std::string(name);

	intret = ipc_->call(ssa::RegistryProtocol::kRemove, ipc_->id(), name_str, r);
	if (intret) {
		return -intret;
	}

	return 0;
}


} // namespace client
} // namespace ssa
