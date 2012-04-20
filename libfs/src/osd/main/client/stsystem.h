#ifndef __STAMNOS_OSD_CLIENT_STORAGE_SYSTEM_H
#define __STAMNOS_OSD_CLIENT_STORAGE_SYSTEM_H

#include "bcs/main/client/bcs-opaque.h"
#include "osd/main/client/osd-opaque.h"
#include "osd/main/common/stsystem.h"
#include "scm/pool/pool.h"

namespace osd {
namespace client {

/**
 * \brief Storage System Abstraction
 *
 * Currently we support a single instance. 
 * To support multiple instances we need to extend the lock managers to 
 * allow multiple instances. The problem is with the RPC handlers.
 *
 */
class StorageSystem {
public:
	StorageSystem(::client::Ipc* ipc)
		: ipc_(ipc)
	{ }

	~StorageSystem();

	int Init();

	::client::Ipc* ipc() { return ipc_; }
	osd::cc::client::HLockManager* hlckmgr() { return hlckmgr_; }
	osd::cc::client::LockManager* lckmgr() { return lckmgr_; }
	StorageAllocator* salloc() { return salloc_; }
	ObjectManager* omgr() { return omgr_; }
	Registry* registry() { return registry_; }
	OsdSharedBuffer* shbuf() { return shbuf_; }

	int Open(const char* source, unsigned int flags);
	int Close();
	int Mount(const char* source, unsigned int flags);
	int Mount(const char* source, unsigned int flags, StorageSystemDescriptor& desc);
	int Mount(const char* source, const char* target, unsigned int flags);
	int Mount(const char* source, const char* target, unsigned int flags, StorageSystemDescriptor& desc);

protected:
	::client::Ipc*                  ipc_;
	StoragePool*                    pool_;
	StorageAllocator*               salloc_;
	OsdSharedBuffer*                shbuf_;
	ObjectManager*                  omgr_;
	Registry*                       registry_;
	osd::cc::client::HLockManager*  hlckmgr_;
	osd::cc::client::LockManager*   lckmgr_;
};

} // namespace client
} // namespace osd


#endif // __STAMNOS_OSD_CLIENT_STORAGE_SYSTEM_H
