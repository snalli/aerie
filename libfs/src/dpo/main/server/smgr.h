#ifndef __STAMNOS_DPO_MAIN_SERVER_STORAGE_MANAGER_H
#define __STAMNOS_DPO_MAIN_SERVER_STORAGE_MANAGER_H

#include <vector>
#include "ipc/ipc.h"
#include "ipc/main/server/cltdsc.h"
#include "dpo/main/server/dpo-opaque.h"
#include "dpo/main/common/storage_protocol.h"
#include "dpo/main/common/obj.h"

namespace server {
class Session;  // forward declaration
} // namespace client


namespace dpo {
namespace server {


const int kMaxStorageDescriptorCount = 8;

class StorageDescriptor; // forward declaration

class StorageManager {
public:
	StorageManager(::server::Ipc* ipc, dpo::server::Dpo* dpo);
	int Init();

	int RegisterPartition(const char* dev, dpo::common::ObjectId stcnr);

	int Alloc(size_t nbytes, std::type_info const& typid, void** ptr);
	int Alloc(::server::Session* session, size_t nbytes, std::type_info const& typid, void** ptr);
	int AllocateRaw(::server::Session* session, size_t size, void** ptr);
	int AllocateContainer(::server::Session* session, int type, int num);

	class IpcHandlers {
	public:
		int Register(StorageManager* smgr);

		int AllocateContainer(int clt, int type, int num, int& r);
		int AllocateContainerVector(int clt, std::vector< ::dpo::StorageProtocol::ContainerRequest> container_request_vector, std::vector<int>& result);

	private:
		StorageManager* smgr_;
	}; 

private:
	::server::Ipc*     ipc_;
	dpo::server::Dpo*  dpo_;
	IpcHandlers        ipc_handlers_;
	StorageDescriptor* descriptor_[kMaxStorageDescriptorCount];
	int                descriptor_count_;
	
};

// holds per storage partition information 
class StorageDesriptor {
public:


private:
	//dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object* stcnr_;
	const char* dev_;
	int ref_cnt_;
};


// holds per client storage session information
struct StorageClientDescriptor: public ::server::ClientDescriptorTemplate<StorageClientDescriptor> {
public:
	StorageClientDescriptor() {
		printf("StorageDescriptor: CONSTRUCTOR: %p\n", this);
	}
	int id;
	int cap;
	
};



} // namespace server
} // namespace dpo



#endif // __STAMNOS_DPO_MAIN_SERVER_MANAGER_H
