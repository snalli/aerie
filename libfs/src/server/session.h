#ifndef __STAMNOS_FS_SERVER_SESSION_H
#define __STAMNOS_FS_SERVER_SESSION_H

#include "dpo/main/server/dpo.h"

namespace server {

class Session {
public:
	Session(dpo::server::Dpo* dpo)
		: dpo_(dpo)
	{ }

	dpo::server::StorageManager* smgr() { return dpo_->smgr(); }

private:
	dpo::server::Dpo* dpo_;
	int               acl;
};


} // namespace server


#endif // __STAMNOS_FS_SERVER_SESSION_H
