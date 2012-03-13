#ifndef __STAMNOS_DPO_SET_CONTAINER_H
#define __STAMNOS_DPO_SET_CONTAINER_H

#include "dpo/containers/set/common.h"
#include "dpo/main/server/session.h"
#include "dpo/main/client/session.h"

namespace dpo {
namespace containers {
namespace client {

template<typename T>
class SetContainer {
public:
	typedef typename dpo::containers::common::SetContainer<T>::template Object< ::client::Session>  Object;
	
};


} // namespace client
} // namespace containers
} // namespace dpo



namespace dpo {
namespace containers {
namespace server {

template<typename T>
class SetContainer {
public:
	//typedef typename dpo::containers::common::SetContainer<T> SetContainerT ;
	//typedef typename dpo::containers::common::SetContainer::Object< ::server::Session>  Object;
	//typename SetContainerT::Object< ::server::Session>  Object;

	
	typedef typename dpo::containers::common::SetContainer<T>::template Object< ::dpo::server::DpoSession>  Object;
	
private:
	//Object* obj_;
};

} // namespace server
} // namespace containers 
} // namespace dpo

#endif // __STAMNOS_DPO_SET_CONTAINER_H
