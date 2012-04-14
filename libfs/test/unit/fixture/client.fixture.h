#ifndef __STAMNOS_TEST_CLIENT_FIXTURE_H
#define __STAMNOS_TEST_CLIENT_FIXTURE_H

#include "pxfs/client/session.h"

namespace osd {
namespace client {

#define __STAMNOS_OSD_CLIENT_STORAGE_ALLOCATOR_H // ugly trick to prevent salloc.h from redefining 

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

	int AllocateContainer(::client::Session* session, int type, osd::common::ObjectId* oid)
	{

	}

};

} // namespace client
} // namespace osd


class PseudoStorageSystem: public osd::client::StorageSystem {
public:
	PseudoStorageSystem(osd::client::StorageAllocator* salloc)
		: StorageSystem(NULL)
	{ 
		salloc_ = salloc;
	}
};

struct ClientFixture 
{
	ClientFixture() 
		: session(NULL)
	{ 
		osd::client::StorageAllocator* salloc = new osd::client::StorageAllocator();
		PseudoStorageSystem* stsystem = new PseudoStorageSystem(salloc);
		session = new client::Session(stsystem);
	}

	~ClientFixture() 
	{
		delete session->salloc_;
		delete session;
	}
	
	client::Session* session;
};

#endif // __STAMNOS_TEST_CLIENT_FIXTURE_H
