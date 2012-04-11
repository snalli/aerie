#ifndef __STAMNOS_FS_CLIENT_SESSION_H
#define __STAMNOS_FS_CLIENT_SESSION_H

#include "osd/main/client/osd-opaque.h"
#include "osd/main/client/session.h"

namespace client {

class Session: public osd::client::OsdSession {
public:
	Session(osd::client::StorageSystem* stsystem)
		: OsdSession(stsystem)
	{ }
};


} // namespace client

#endif // __STAMNOS_FS_CLIENT_SESSION_H
