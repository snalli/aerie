#ifndef __STAMNOS_DPO_CLIENT_DPO_LAYER_H
#define __STAMNOS_DPO_CLIENT_DPO_LAYER_H

namespace client {
class Ipc;       // forward declaration
} // namespace client


namespace dpo {

namespace cc {
namespace client {
class LockManager;    // forward declaration
class HLockManager;   // forward declaration
} // namespace client
} // namespace cc


namespace client {

class ObjectManager;  // forward declaration
class StorageManager; // forward declaration

// Distributed Persistent Object Layer
class Dpo {
public:
	Dpo(::client::Ipc* ipc)
		: ipc_(ipc)
	{ }

	~Dpo();

	int Init();

	dpo::cc::client::HLockManager* hlckmgr() { return hlckmgr_; }
	dpo::cc::client::LockManager* lckmgr() { return lckmgr_; }
	StorageManager* smgr() { return smgr_; }
	ObjectManager* omgr() { return omgr_; }

private:
	::client::Ipc*                  ipc_;
	StorageManager*                 smgr_;
	ObjectManager*                  omgr_;
	dpo::cc::client::HLockManager*  hlckmgr_;
	dpo::cc::client::LockManager*   lckmgr_;
};

} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_DPO_LAYER_H
