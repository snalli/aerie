#ifndef __TEST_CLIENT_FIXTURE_H_JAL190
#define __TEST_CLIENT_FIXTURE_H_JAL190

#include "client/session.h"


namespace client {

#define __CLIENT_STORAGE_MANAGER_H_AKL783 // ugly trick to prevent smgr.h from redefining 
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
		session = new client::Session(smgr);
	}

	~ClientFixture() 
	{
		delete session->smgr_;
		delete session;
	}
	
	client::Session* session;
};

#endif // __TEST_CLIENT_FIXTURE_H_JAL190
