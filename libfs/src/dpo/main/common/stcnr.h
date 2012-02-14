#ifndef __STAMNOS_DPO_COMMON_STORAGE_CONTAINER_H
#define __STAMNOS_DPO_COMMON_STORAGE_CONTAINER_H

#include "dpo/main/common/obj.h"
#include "dpo/containers/byte/common.h"

namespace dpo {
namespace containers
namespace common {


template<typename Session>
class StorageContainer {
	typedef dpo::containers::common::ByteContainer::Object< ::server::Session>  ByteContainer;

public:
	

private:	
	ByteContainer byte_container_;
};


} // namespace common
} // namespace containers
} // namespace dpo

#endif // __STAMNOS_DPO_COMMON_STORAGE_CONTAINER_H
