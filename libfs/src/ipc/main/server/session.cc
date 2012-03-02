#include "ipc/main/server/ipc.h"
#include "ipc/main/server/session.h"
#include "ipc/main/server/cltdsc.h"
#include "common/errno.h"

namespace server {

int
IpcSession::Init(int clt, Ipc* ipc)
{
	ClientDescriptor* cltdsc;
	if ((cltdsc = ipc->Client(clt)) == NULL) {
		return -E_NOENT;
	}
	ipc_ = ipc;
	return E_SUCCESS;
}


} // namespace server
