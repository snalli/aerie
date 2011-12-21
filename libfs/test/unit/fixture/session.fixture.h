#ifndef __STAMNOS_TEST_SESSION_FIXTURE_H
#define __STAMNOS_TEST_SESSION_FIXTURE_H

class Session;

class StorageManager {
public:
	int Alloc(Session* session, size_t nbytes, std::type_info const& typid, void** ptr)
	{
		*ptr = malloc(nbytes);
		return E_SUCCESS;
	}
};

class Session {
public:
	StorageManager* smgr_;
};

struct SessionFixture 
{
	SessionFixture() 
		: session(NULL)
	{ 
		session = new Session();
		session->smgr_ = new StorageManager();
	}

	~SessionFixture() 
	{
		delete session->smgr_;
		delete session;
	}
	
	Session* session;
};

#endif // __STAMNOS_TEST_SESSION_FIXTURE_H
