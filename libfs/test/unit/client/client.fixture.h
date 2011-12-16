#ifndef __STAMNOS_TEST_CLIENT_FIXTURE_H
#define __STAMNOS_TEST_CLIENT_FIXTURE_H

#include "client/session.h"


namespace client {

#define __STAMNOS_FS_CLIENT_STORAGE_MANAGER_H // ugly trick to prevent smgr.h from redefining 
                                              // StorageManager
class StorageManager {
public:
	int Alloc(client::Session* session, size_t nbytes, std::type_info const& typid, void** ptr)
	{
		*ptr = malloc(nbytes);
		return E_SUCCESS;
	}
};

} // namespace client

struct ClientFixture 
{
	ClientFixture() 
		: session(NULL)
	{ 
		client::StorageManager* smgr = new client::StorageManager();
		session = new client::Session(NULL, NULL, smgr);
	}

	~ClientFixture() 
	{
		delete session->smgr_;
		delete session;
	}
	
	client::Session* session;
};

#endif // __STAMNOS_TEST_CLIENT_FIXTURE_H
