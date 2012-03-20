#include "ssa/main/server/session.h"
#include "ssa/main/server/ssa.h"
#include "ssa/main/server/shbuf.h"
#include "bcs/bcs.h"
#include "common/errno.h"

namespace ssa {
namespace server {

int 
SsaSession::Init(int clt)
{
	int  ret;
	char buf[64];

	if ((ret = ::server::BcsSession::Init(clt)) < 0) {
		return ret;
	}
	sets_.clear();

	// create and init shared buffer
	if ((shbuf_ = reinterpret_cast<SsaSharedBuffer*>(ipc_->shbuf_manager()->CreateSharedBuffer("SsaSharedBuffer", static_cast<BcsSession*>(this)))) == NULL) {
		return -E_NOMEM;
	}
	sprintf(buf, "%d", clt);

	if ((ret = shbuf_->Init(buf)) < 0) {
		return ret;
	}

	return E_SUCCESS;
}


ssa::server::StorageAllocator* 
SsaSession::salloc() 
{ 
	return storage_system_->salloc(); 
} 

} // namespace server
} // namespace ssa
