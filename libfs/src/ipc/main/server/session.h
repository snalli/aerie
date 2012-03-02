#ifndef __STAMNOS_IPC_SERVER_SESSION_H
#define __STAMNOS_IPC_SERVER_SESSION_H

#include "ipc/main/server/ipc.h"

namespace server {


// Implements the base interface as expected by the Base Session Manager.
class BaseSession {


};


// IPC layer specific session
class IpcSession: public BaseSession {
public:

	int Init(int clt, Ipc* ipc);

protected:
	Ipc* ipc_;
};


} // namespace server

#endif // __STAMNOS_IPC_SERVER_SESSION_H
