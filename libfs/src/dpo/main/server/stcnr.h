#ifndef __STAMNOS_DPO_MAIN_SERVER_STORAGE_CONTAINER_H
#define __STAMNOS_DPO_MAIN_SERVER_STORAGE_CONTAINER_H

#include "dpo/main/common/stcnr.h"
#include "dpo/main/server/session.h"

namespace server {


class StorageContainer 
	typedef dpo::containers::common::StorageContainer::Object< ::server::Session>  Object;

public:
	
	
private:
	Object* obj_;
};


} // namespace server

#endif // __STAMNOS_DPO_MAIN_SERVER_STORAGE_CONTAINER_H
