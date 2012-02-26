#ifndef __STAMNOS_TEST_SESSION_FIXTURE_H
#define __STAMNOS_TEST_SESSION_FIXTURE_H

class Session;

class StorageAllocator {
public:
	int AllocateRaw(Session* session, size_t nbytes, void** ptr)
	{
		*ptr = malloc(nbytes);
		return E_SUCCESS;
	}

	int Alloc(Session* session, size_t nbytes, std::type_info const& typid, void** ptr)
	{
		*ptr = malloc(nbytes);
		return E_SUCCESS;
	}
};

class Session {
public:
	StorageAllocator* salloc() { return salloc_; };
	StorageAllocator* salloc_;
};

struct SessionFixture 
{
	SessionFixture() 
		: session(NULL)
	{ 
		session = new Session();
		session->salloc_ = new StorageAllocator();
	}

	~SessionFixture() 
	{
		delete session->salloc_;
		delete session;
	}
	
	Session* session;
};

#endif // __STAMNOS_TEST_SESSION_FIXTURE_H
