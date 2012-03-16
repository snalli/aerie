#ifndef __STAMNOS_SSA_SET_CONTAINER_H
#define __STAMNOS_SSA_SET_CONTAINER_H

#include "ssa/containers/set/common.h"
#include "ssa/main/server/session.h"
#include "ssa/main/client/session.h"

namespace ssa {
namespace containers {
namespace client {

template<typename T>
class SetContainer {
public:
	typedef typename ssa::containers::common::SetContainer<T>::template Object< ::client::Session>  Object;
	
};


} // namespace client
} // namespace containers
} // namespace ssa



namespace ssa {
namespace containers {
namespace server {

template<typename T>
class SetContainer {
public:
	//typedef typename ssa::containers::common::SetContainer<T> SetContainerT ;
	//typedef typename ssa::containers::common::SetContainer::Object< ::server::Session>  Object;
	//typename SetContainerT::Object< ::server::Session>  Object;

	
	typedef typename ssa::containers::common::SetContainer<T>::template Object< ::ssa::server::DpoSession>  Object;
	
private:
	//Object* obj_;
};

} // namespace server
} // namespace containers 
} // namespace ssa

#endif // __STAMNOS_SSA_SET_CONTAINER_H
