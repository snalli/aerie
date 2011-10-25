#ifndef __SESSION_FIXTURE_H_JAL190
#define __SESSION_FIXTURE_H_JAL190

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
	StorageManager* sm;
};

struct SessionFixture 
{
	SessionFixture() 
		: session(NULL)
	{ 
		session = new Session();
		session->sm = new StorageManager();
	}

	~SessionFixture() 
	{
		delete session->sm;
		delete session;
	}
	
	Session* session;
};


#endif // __SESSION_FIXTURE_H_JAL190
