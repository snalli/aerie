#ifndef __STAMNOS_TEST_SESSION_FIXTURE_H
#define __STAMNOS_TEST_SESSION_FIXTURE_H

#include "osd/main/common/obj.h"

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

	int AllocateExtent(Session* session, size_t nbytes, int flags, void** ptr)
	{
		*ptr = malloc(nbytes);
		return E_SUCCESS;
	}

	int AllocateContainer(Session* session, int type, osd::common::ObjectId* oid)
	{

	}
};

class PseudoJournal {
public:
	int TransactionBegin(int id = 0) { return E_SUCCESS; }
	int TransactionCommit() { return E_SUCCESS; }

	template<typename T>
	void Store(volatile T* addr, T val)
	{
		*addr = val;
	}
};

class Session {
public:
	StorageAllocator* salloc() { return salloc_; };
	StorageAllocator* salloc_;
	PseudoJournal* journal() { return journal_; };
	PseudoJournal* journal_;
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
