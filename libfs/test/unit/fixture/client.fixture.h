#ifndef __STAMNOS_TEST_CLIENT_FIXTURE_H
#define __STAMNOS_TEST_CLIENT_FIXTURE_H

#include "client/session.h"

namespace ssa {
namespace client {

#define __STAMNOS_DPO_CLIENT_STORAGE_ALLOCATOR_H // ugly trick to prevent salloc.h from redefining 

// StorageAllocator
class StorageAllocator {
public:
	int Alloc(::client::Session* session, size_t nbytes, std::type_info const& typid, void** ptr)
	{
		*ptr = malloc(nbytes);
		return E_SUCCESS;
	}
	
	int AllocateExtent(::client::Session* session, size_t nbytes, void** ptr)
	{
		*ptr = malloc(nbytes);
		return E_SUCCESS;
	}

	int AllocateContainer(::client::Session* session, int type, ssa::common::ObjectId* oid)
	{

	}

};

} // namespace client
} // namespace ssa

struct ClientFixture 
{
	ClientFixture() 
		: session(NULL)
	{ 
		ssa::client::StorageAllocator* salloc = new ssa::client::StorageAllocator();
		session = new client::Session(NULL, NULL, salloc, NULL);
	}

	~ClientFixture() 
	{
		delete session->salloc_;
		delete session;
	}
	
	client::Session* session;
};

#endif // __STAMNOS_TEST_CLIENT_FIXTURE_H
