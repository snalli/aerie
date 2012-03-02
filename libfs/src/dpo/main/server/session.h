#ifndef __STAMNOS_DPO_SERVER_SESSION_H
#define __STAMNOS_DPO_SERVER_SESSION_H

#include "dpo/main/server/dpo-opaque.h"

namespace dpo {
namespace server {

class DpoSession {
public:

	dpo::server::StorageAllocator* salloc() { /*TODO*/} 

	int Init(int clt, dpo::server::Dpo* dpo);

protected:
	dpo::server::Dpo* dpo_;
};


} // namespace dpo
} // namespace server

#endif // __STAMNOS_DPO_SERVER_SESSION_H
