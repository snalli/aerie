#ifndef __STAMNOS_FS_CLIENT_SESSION_H
#define __STAMNOS_FS_CLIENT_SESSION_H

#include "dpo/client/stm.h"

namespace client {

class StorageManager;

class Session {
public:
	Session(client::StorageManager* smgr)
		: smgr_(smgr)
	{ }

	client::StorageManager* smgr_;
	stm::Transaction*       tx_;
};

} // namespace client

#endif // __STAMNOS_FS_CLIENT_SESSION_H
