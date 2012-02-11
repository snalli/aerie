#ifndef __STAMNOS_DPO_SERVER_DPO_LAYER_H
#define __STAMNOS_DPO_SERVER_DPO_LAYER_H

#include "ipc/ipc.h"

namespace dpo {

namespace cc {
namespace server {
class LockManager;    // forward declaration
class HLockManager;   // forward declaration
} // namespace server
} // namespace cc


namespace server {

class ObjectManager;  // forward declaration
class StorageManager; // forward declaration

// Distributed Persistent Object Layer
class Dpo {
public:
	Dpo(::server::Ipc* ipc)
		: ipc_(ipc)
	{ }

	~Dpo();

	int Init();

	dpo::cc::server::HLockManager* hlckmgr() { return hlckmgr_; }
	dpo::cc::server::LockManager* lckmgr() { return lckmgr_; }
	StorageManager* smgr() { return smgr_; }

private:
	::server::Ipc*                  ipc_;
	dpo::cc::server::HLockManager*  hlckmgr_;
	dpo::cc::server::LockManager*   lckmgr_;
	StorageManager*                 smgr_;
};

} // namespace server
} // namespace dpo

#endif // __STAMNOS_DPO_SERVER_DPO_LAYER_H
