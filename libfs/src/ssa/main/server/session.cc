#include "ssa/main/server/session.h"
#include "ssa/main/server/ssa.h"
#include "bcs/bcs.h"
#include "common/errno.h"

namespace ssa {
namespace server {

int 
SsaSession::Init(int clt)
{
	int ret;

	if ((ret = ::server::BcsSession::Init(clt)) < 0) {
		return ret;
	}
	sets_.clear();
	return E_SUCCESS;
}


ssa::server::StorageAllocator* 
SsaSession::salloc() 
{ 
	return storage_system_->salloc(); 
} 

} // namespace server
} // namespace ssa