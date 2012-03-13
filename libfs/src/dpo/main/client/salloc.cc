#include "dpo/main/client/salloc.h"
#include <stdlib.h>
#include <typeinfo>
#include <vector>
#include "common/errno.h"
#include "common/debug.h"
#include "ipc/ipc.h"
#include "dpo/main/common/storage_protocol.h"
#include "dpo/containers/set/container.h"
#include "client/session.h"



namespace dpo {
namespace client {


// Storage manager
// It allocates storage for a new object:
// 1) from a local pool of objects (that match the ACL), or
// 2) from the kernel storage manager, or
// 3) by contacting the file system server
//
// the allocation function should take a transaction id as argument
// to ensure the atomicity of the operation
//


int 
StorageAllocator::Load(StoragePool* pool) 
{
	pool_ = pool;
	return E_SUCCESS;
}


int
StorageAllocator::AllocateExtent(::client::Session* session, size_t nbytes, void** ptr)
{
	return pool_->AllocateExtent(nbytes, ptr);
}



// OBSOLETE
int 
StorageAllocator::Alloc(size_t nbytes, std::type_info const& typid, void** ptr)
{
	assert(0);
}


// OBSOLETE
int 
StorageAllocator::Alloc(::client::Session* session, size_t nbytes, std::type_info const& typid, void** ptr)
{
	assert(0);
	//FIXME
	/*
	ChunkDescriptor* achunkdsc[16];
	size_t           roundup_bytes = (nbytes % 4096 == 0) ? nbytes: ((nbytes/4096)+1)*4096;

	//chunk_store.AccessChunkList(achunkdsc, 1, PROT_READ|PROT_WRITE);
	chunk_store.CreateChunk(roundup_bytes, CHUNK_TYPE_INODE, &achunkdsc[0]);
	*ptr = achunkdsc[0]->chunk_;

	return 0;
	*/
}


int 
StorageAllocator::AllocateContainer(::client::Session* session, int type, dpo::common::ObjectId* ret_oid)
{
	int                                                                   ret;
	::dpo::StorageProtocol::ContainerReply                                reply;
	dpo::containers::client::SetContainer<dpo::common::ObjectId>::Object* set_obj;
	dpo::common::ObjectId                                                 set_oid;
	dpo::common::ObjectId                                                 oid;
	ObjectIdSet*                                                          freeset;
	ObjectIdSet::iterator                                                 it;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_salloc), 
	        "[%d] Allocate container of type %d\n", ipc_->id(), type);

	if (freeset_[type] && freeset_[type]->size() > 0) {
		freeset = freeset_[type];
	} else {
		if ((ret = ipc_->call(dpo::StorageProtocol::kAllocateContainer, 
							  ipc_->id(), type, 8, reply)) < 0) {
			return ret;
		} else if (ret > 0) {
			return -ret;
		}
		set_oid = reply.oid;
		if ((set_obj = dpo::containers::client::SetContainer<dpo::common::ObjectId>::Object::Load(set_oid)) == NULL) {
            return -1;
        } 
		if ((freeset = freeset_[type]) == NULL) {
			freeset = freeset_[type] = new ObjectIdSet();
			freeset->set_empty_key(dpo::common::ObjectId(0));
			freeset->set_deleted_key(dpo::common::ObjectId(1));
		}
		for (int i=0; i<set_obj->Size(); i++) {
			set_obj->Read(session, i, &oid);
			freeset->insert(oid);
		}
	}
	it = freeset->begin();
	*ret_oid = *it;
	freeset->erase(it);
	return E_SUCCESS;
}


int 
StorageAllocator::AllocateContainerVector(::client::Session* session)
{
	int                                                    ret;
	int                                                    r;
	std::vector< ::dpo::StorageProtocol::ContainerRequest> container_req_vec;
	std::vector<int>                                       rv;
	std::vector<int>::iterator                             rvi;

	::dpo::StorageProtocol::ContainerRequest req;

	req.type = 1;
	req.num = 2;

	container_req_vec.push_back(req);

	ret = ipc_->call(dpo::StorageProtocol::kAllocateContainerVector, 
	                 ipc_->id(), container_req_vec, rv);

	for (rvi = rv.begin(); rvi != rv.end(); rvi++) {
		printf("%d\n", *rvi);
	}
}


} // namespace client
} // namespace dpo
