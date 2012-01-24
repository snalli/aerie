#ifndef __STAMNOS_DPO_COMMON_OBJECT_PROXY_H
#define __STAMNOS_DPO_COMMON_OBJECT_PROXY_H

#include <pthread.h>
#include "common/list.h"
#include "dpo/base/common/obj.h"


namespace dpo {

namespace common {

class ObjectProxy; // forward reference


class ObjectProxyReference {
friend class ObjectProxy;
public:
	ObjectProxyReference() 
		: obj_(NULL)
	{ }

	ObjectProxy* obj() { 
		return obj_;
	}
	inline void Set(ObjectProxy* obj, bool lock = true);
	inline void Reset(bool lock = true);

protected:
	ObjectProxy*     obj_;
	struct list_head reflist_;
};


// this serves as the base class type
class ObjectProxy {
friend class ObjectProxyReference;
public:
	ObjectProxy()
		: refcnt_(0),
		  subject_(NULL)  
	{
		pthread_mutex_init(&mutex_, NULL);
		INIT_LIST_HEAD(&reflist_);
	}

	ObjectProxy(dpo::common::ObjectId oid)
		: refcnt_(0)
	{
		pthread_mutex_init(&mutex_, NULL);
		INIT_LIST_HEAD(&reflist_);
		subject_ = static_cast<dpo::common::Object*>(oid.addr());
	}

	dpo::common::ObjectId oid() {
		return subject_->oid();
	}

	ObjectProxyReference* HeadReference() {
		ObjectProxyReference* ref = NULL;
		pthread_mutex_lock(&mutex_);
		if (refcnt_ > 0) {
			struct list_head* first_entry = (&reflist_)->next;
			int off = offsetof(ObjectProxyReference, reflist_);
			//ref = (ObjectProxyReference*)((char*) first_entry - offsetof(ObjectProxyReference, reflist_));
		} 
		pthread_mutex_unlock(&mutex_);
		return ref;
	}

protected:
	pthread_mutex_t      mutex_;
	struct list_head     reflist_;   // reference list 
	int                  refcnt_;
	dpo::common::Object* subject_;
};


void ObjectProxyReference::Set(ObjectProxy* obj, bool lock) {
	if (lock) {
		pthread_mutex_lock(&(obj->mutex_));
	}
	assert(obj_ == NULL);
	obj_ = obj;
	obj_->refcnt_++;
	list_add_tail(&reflist_, &(obj->reflist_));
	if (lock) {
		pthread_mutex_unlock(&(obj->mutex_));
	}
}

void ObjectProxyReference::Reset(bool lock) {
	if (obj_ == NULL) {
		return;
	}
	if (lock) {
		pthread_mutex_lock(&(obj_->mutex_));
	}
	obj_->refcnt_++;
	list_del(&reflist_);
	if (lock) {
		pthread_mutex_unlock(&(obj_->mutex_));
	}
	obj_ = NULL;
}



 
} // namespace common
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H
