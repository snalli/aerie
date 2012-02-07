#ifndef __STAMNOS_DPO_CLIENT_DPO_LAYER_H
#define __STAMNOS_DPO_CLIENT_DPO_LAYER_H

#include "rpc/rpc.h"

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
class DpoLayer {
public:
	DpoLayer(rpcc* rpc_client, rpcs* rpc_server, std::string id, int principal_id)
		: rpc_client_(rpc_client),
		  rpc_server_(rpc_server),
		  id_(id),
		  principal_id_(principal_id)
	{ }

	~DpoLayer();

	int Init();

	dpo::cc::client::HLockManager* hlckmgr() { return hlckmgr_; }
	dpo::cc::client::LockManager* lckmgr() { return lckmgr_; }
	StorageManager* smgr() { return smgr_; }
	ObjectManager* omgr() { return omgr_; }

private:
	rpcc*                           rpc_client_;
	rpcs*                           rpc_server_;
	std::string                     id_;
	int                             principal_id_;
	dpo::cc::client::HLockManager*  hlckmgr_;
	dpo::cc::client::LockManager*   lckmgr_;
	StorageManager*                 smgr_;
	ObjectManager*                  omgr_;
};

} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_DPO_LAYER_H
