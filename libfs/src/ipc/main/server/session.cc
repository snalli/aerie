#include "ipc/main/server/ipc.h"
#include "ipc/main/server/session.h"
#include "common/errno.h"

namespace server {

int
IpcSession::Init(int clt)
{
	if ((cltdsc_ = ipc_->Client(clt)) == NULL) {
		return -E_NOENT;
	}
	return E_SUCCESS;
}


} // namespace server
