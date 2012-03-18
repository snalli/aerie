#ifndef __STAMNOS_BCS_SERVER_SESSION_H
#define __STAMNOS_BCS_SERVER_SESSION_H

#include "bcs/main/server/ipc.h"
#include "bcs/main/server/cltdsc.h"

namespace server {


// Implements the base interface as expected by the Base Session Manager.
class BaseSession {


};


// BCS layer specific session
class BcsSession: public BaseSession {
public:

	int Init(int clt);
	int clt() { return cltdsc_->clt(); }

protected:
	Ipc*              ipc_;
	ClientDescriptor* cltdsc_;
};


} // namespace server

#endif // __STAMNOS_BCS_SERVER_SESSION_H
