#ifndef __STAMNOS_DPO_CLIENT_STORAGE_MANAGER_H
#define __STAMNOS_DPO_CLIENT_STORAGE_MANAGER_H

#include <stdlib.h>
#include <typeinfo>
#include "ipc/main/client/ipc-opaque.h"


namespace client {
class Session;  // forward declaration
} // namespace client


namespace dpo {
namespace client {

class StorageManager {
public:
	StorageManager(::client::Ipc* ipc)
		: ipc_(ipc)
	{ }	

	int AllocateRaw(::client::Session* session, size_t nbytes, void** ptr);
	int Alloc(size_t nbytes, std::type_info const& typid, void** ptr);
	int Alloc(::client::Session* session, size_t nbytes, std::type_info const& typid, void** ptr);
	int AllocExtent(::client::Session* session, size_t nbytes, void** ptr);

	int AllocateContainer(::client::Session* session);
	int AllocateContainerVector(::client::Session* session);
private:
	::client::Ipc*  ipc_;
};

} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_STORAGE_MANAGER_H
