#include "bcs/main/server/shbuf.h"
#include "bcs/main/common/debug.h"
#include "bcs/main/server/ipc.h"

namespace server {


int
SharedBuffer::Init()
{
	int    ret;
	
	DBG_LOG(DBG_INFO, DBG_MODULE(server_bcs), "SharedBuffer size = %ld\n", Ipc::RuntimeConfig::sharedbuffer_size);

	
	

}


} // namespace server
