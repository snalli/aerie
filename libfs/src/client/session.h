#ifndef _CLIENT_SESSION_H_AKU197
#define _CLIENT_SESSION_H_AKU197

#include "client/smgr.h"

namespace client {

class ClientSession {
public:
	ClientSession(client::StorageManager* smgr)
		: sm(smgr)
	{ }

	client::StorageManager* sm;
};

} // namespace client

#endif /* _CLIENT_SESSION_H_AKU197 */
