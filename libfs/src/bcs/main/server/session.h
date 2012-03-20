#ifndef __STAMNOS_BCS_SERVER_SESSION_H
#define __STAMNOS_BCS_SERVER_SESSION_H

#include <vector>
#include "bcs/main/server/ipc.h"
#include "bcs/main/server/cltdsc.h"
#include "bcs/main/server/bcs-opaque.h"

namespace server {


// Implements the base interface as expected by the Base Session Manager.
class BaseSession {


};


// BCS layer specific session
class BcsSession: public BaseSession {
friend class SharedBufferManager;
public:

	int Init(int clt);
	int clt() { return cltdsc_->clt(); }

protected:
	Ipc*                       ipc_;
	ClientDescriptor*          cltdsc_;
	std::vector<SharedBuffer*> shbuf_vec_;
};


} // namespace server

#endif // __STAMNOS_BCS_SERVER_SESSION_H
