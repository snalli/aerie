#ifndef __STAMNOS_OSD_CLIENT_REGISTRY_H
#define __STAMNOS_OSD_CLIENT_REGISTRY_H

#include <sys/types.h>
#include "osd/main/common/obj.h"
#include "bcs/bcs-opaque.h"


namespace osd {
namespace client {

class Registry {
public:
	Registry(::client::Ipc* ipc);
	int Lookup(const char *name, osd::common::ObjectId* oid);
	int Add(const char *name, osd::common::ObjectId oid);
	int Remove(const char *name);

private:
	::client::Ipc* ipc_;
};

} // namespace client
} // namespace osd

#endif // __STAMNOS_OSD_CLIENT_REGISTRY_H
