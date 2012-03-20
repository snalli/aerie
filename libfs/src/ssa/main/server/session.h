#ifndef __STAMNOS_SSA_SERVER_SESSION_H
#define __STAMNOS_SSA_SERVER_SESSION_H

#include "ssa/main/common/obj.h"
#include "bcs/main/server/session.h"
#include "ssa/main/server/ssa-opaque.h"
#include "ssa/main/server/shbuf.h"

namespace ssa {
namespace server {

/**< Storage System Abstractions layer session */
class SsaSession: public ::server::BcsSession {
public:

	ssa::server::StorageAllocator* salloc();

	int Init(int clt);

//protected:
	ssa::server::SsaSharedBuffer*      shbuf_;
	ssa::server::StorageSystem*        storage_system_;
	std::vector<ssa::common::ObjectId> sets_; // sets of pre-allocated containers to client
};


} // namespace ssa
} // namespace server

#endif // __STAMNOS_SSA_SERVER_SESSION_H
