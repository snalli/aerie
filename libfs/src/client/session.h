#ifndef _CLIENT_SESSION_H_AKU197
#define _CLIENT_SESSION_H_AKU197

#include "dcc/client/stm.h"

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

#endif /* _CLIENT_SESSION_H_AKU197 */
