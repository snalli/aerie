#include "osd/main/server/session.h"
#include "osd/main/server/osd.h"
#include "osd/main/server/shbuf.h"
#include "osd/main/server/journal.h"
#include "bcs/bcs.h"
#include "common/errno.h"

namespace osd {
namespace server {

int 
OsdSession::Init(int clt)
{
	int  ret;
	char buf[64];

	if ((ret = ::server::BcsSession::Init(clt)) < 0) {
		return ret;
	}
	sets_.clear();

	// create and init shared buffer
	if ((shbuf_ = reinterpret_cast<OsdSharedBuffer*>(ipc_->shbuf_manager()->CreateSharedBuffer("OsdSharedBuffer", static_cast<BcsSession*>(this)))) == NULL) {
		return -E_NOMEM;
	}
	sprintf(buf, "%d", clt);

	if ((ret = shbuf_->Init(buf)) < 0) {
		return ret;
	}
	
	journal_ = new Journal();
	return E_SUCCESS;
}


osd::server::StorageAllocator* 
OsdSession::salloc() 
{ 
	return storage_system_->salloc(); 
} 

} // namespace server
} // namespace osd
