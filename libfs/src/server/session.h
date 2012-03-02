#ifndef __STAMNOS_FS_SERVER_SESSION_H
#define __STAMNOS_FS_SERVER_SESSION_H

#include "dpo/main/server/dpo.h"
#include "dpo/main/server/session.h"

namespace server {

class Session: public dpo::server::DpoSession {
public:
	Session()
	{ }

	int Init(int clt, dpo::server::Dpo* dpo);

private:
	int               acl;
};


} // namespace server


#endif // __STAMNOS_FS_SERVER_SESSION_H
