#ifndef __STAMNOS_SSA_SERVER_SESSION_H
#define __STAMNOS_SSA_SERVER_SESSION_H

#include "ssa/main/common/obj.h"
#include "ipc/main/server/session.h"
#include "ssa/main/server/ssa-opaque.h"

namespace ssa {
namespace server {

class DpoSession: public ::server::IpcSession {
public:

	ssa::server::StorageAllocator* salloc();

	int Init(int clt);

//protected:
	ssa::server::Dpo*                  ssa_;
	std::vector<ssa::common::ObjectId> sets_; // sets of pre-allocated containers to client
};


} // namespace ssa
} // namespace server

#endif // __STAMNOS_SSA_SERVER_SESSION_H
