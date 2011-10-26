#ifndef _CLIENT_SESSION_H_AKU197
#define _CLIENT_SESSION_H_AKU197


namespace client {

class StorageManager;

class Session {
public:
	Session(client::StorageManager* smgr)
		: sm(smgr)
	{ }

	client::StorageManager* sm;
};

} // namespace client

#endif /* _CLIENT_SESSION_H_AKU197 */
