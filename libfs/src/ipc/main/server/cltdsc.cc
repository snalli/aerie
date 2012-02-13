#include "ipc/main/server/cltdsc.h"
#include <stdio.h>
#include "ipc/backend/rpc.h"

namespace server {

int ClientDescriptor::payload_size_ = 0;
int ClientDescriptor::registered_dsc_cnt_ = 0;
int ClientDescriptor::offset_[16];
AbstractClientDescriptorConstructor* ClientDescriptor::constructors_[16];



/*

main() 
{
	StorageDescriptor::Register();
	LockDescriptor::Register();

	ClientDescriptor* dsc = ClientDescriptor::Create();
	printf("dsc=%p\n", dsc);
	dsc = ClientDescriptor::Create();
	printf("dsc=%p\n", dsc);

	StorageDescriptor* sdsc = StorageDescriptor::Descriptor(dsc);
	printf("sdsc=%p\n", sdsc);

}


*/


} // namespace server
