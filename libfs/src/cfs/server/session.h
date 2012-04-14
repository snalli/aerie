#ifndef __STAMNOS_FS_SERVER_SESSION_H
#define __STAMNOS_FS_SERVER_SESSION_H

#include "osd/main/server/stsystem.h"
#include "osd/main/server/osd-opaque.h"
#include "osd/main/server/session.h"
#include "bcs/main/server/bcs.h"
#include "cfs/common/shreg.h"

namespace server {

class NameSpace; // forward declaration

class Session: public osd::server::OsdSession {
public:
	Session(Ipc* ipc, osd::server::StorageSystemT<Session>* storage_system)
	{ 
		ipc_ = ipc;
		storage_system_ = storage_system;
	}

	int Init(int clt);

	NameSpace*        namespace_;
	SharedRegion*     shreg_;

private:
	int               acl;

};


} // namespace server


#endif // __STAMNOS_FS_SERVER_SESSION_H
