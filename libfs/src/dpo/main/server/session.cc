#include "dpo/main/server/session.h"
#include "dpo/main/server/dpo.h"
#include "ipc/main/server/session.h"
#include "common/errno.h"

namespace dpo {
namespace server {

int 
DpoSession::Init(int clt)
{
	int ret;

	if ((ret = ::server::IpcSession::Init(clt)) < 0) {
		return ret;
	}
	sets_.clear();
	return E_SUCCESS;
}


dpo::server::StorageAllocator* 
DpoSession::salloc() 
{ 
	return dpo_->salloc(); 
} 

} // namespace server
} // namespace dpo
