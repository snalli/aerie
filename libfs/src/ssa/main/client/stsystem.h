#ifndef __STAMNOS_SSA_CLIENT_STORAGE_SYSTEM_H
#define __STAMNOS_SSA_CLIENT_STORAGE_SYSTEM_H

#include "bcs/main/client/bcs-opaque.h"
#include "ssa/main/client/ssa-opaque.h"
#include "spa/pool/pool.h"

namespace ssa {
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
	ssa::cc::client::HLockManager* hlckmgr() { return hlckmgr_; }
	ssa::cc::client::LockManager* lckmgr() { return lckmgr_; }
	StorageAllocator* salloc() { return salloc_; }
	ObjectManager* omgr() { return omgr_; }
	Registry* registry() { return registry_; }
	SsaSharedBuffer* shbuf() { return shbuf_; }

	int Open(const char* source, unsigned int flags);
	int Close();
	int Mount(const char* source, const char* target, unsigned int flags);

protected:
	::client::Ipc*                  ipc_;
	StoragePool*                    pool_;
	StorageAllocator*               salloc_;
	SsaSharedBuffer*                shbuf_;
	ObjectManager*                  omgr_;
	Registry*                       registry_;
	ssa::cc::client::HLockManager*  hlckmgr_;
	ssa::cc::client::LockManager*   lckmgr_;
};

} // namespace client
} // namespace ssa


#endif // __STAMNOS_SSA_CLIENT_STORAGE_SYSTEM_H
