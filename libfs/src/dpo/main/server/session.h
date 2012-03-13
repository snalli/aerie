#ifndef __STAMNOS_DPO_SERVER_SESSION_H
#define __STAMNOS_DPO_SERVER_SESSION_H

#include "dpo/main/common/obj.h"
#include "ipc/main/server/session.h"
#include "dpo/main/server/dpo-opaque.h"

namespace dpo {
namespace server {

class DpoSession: public ::server::IpcSession {
public:

	dpo::server::StorageAllocator* salloc();

	int Init(int clt);

//protected:
	dpo::server::Dpo*                  dpo_;
	std::vector<dpo::common::ObjectId> sets_; // sets of pre-allocated containers to client
};


} // namespace dpo
} // namespace server

#endif // __STAMNOS_DPO_SERVER_SESSION_H
