#ifndef __STAMNOS_SSA_CLIENT_SSA_LAYER_H
#define __STAMNOS_SSA_CLIENT_SSA_LAYER_H

#include "bcs/main/client/bcs-opaque.h"
#include "ssa/main/client/ssa-opaque.h"
#include "spa/pool/pool.h"

namespace ssa {
namespace client {

// Distributed Persistent Object Layer
class Dpo {
public:
	Dpo(::client::Ipc* ipc)
		: ipc_(ipc)
	{ }

	~Dpo();

	int Init();

	ssa::cc::client::HLockManager* hlckmgr() { return hlckmgr_; }
	ssa::cc::client::LockManager* lckmgr() { return lckmgr_; }
	StorageAllocator* salloc() { return salloc_; }
	ObjectManager* omgr() { return omgr_; }
	Registry* registry() { return registry_; }

	int Open(const char* source, unsigned int flags);
	int Close();

private:
	::client::Ipc*                  ipc_;
	StoragePool*                    pool_;
	StorageAllocator*               salloc_;
	ObjectManager*                  omgr_;
	Registry*                       registry_;
	ssa::cc::client::HLockManager*  hlckmgr_;
	ssa::cc::client::LockManager*   lckmgr_;
};

} // namespace client
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_SSA_LAYER_H
