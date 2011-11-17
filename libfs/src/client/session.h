#ifndef _CLIENT_SESSION_H_AKU197
#define _CLIENT_SESSION_H_AKU197


namespace client {

class StorageManager;

class Session {
public:
	Session(client::StorageManager* smgr)
		: smgr_(smgr)
	{ }

	client::StorageManager* smgr_;
};

} // namespace client

#endif /* _CLIENT_SESSION_H_AKU197 */
