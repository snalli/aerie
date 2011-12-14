#ifndef __STAMNOS_DPO_COMMON_OBJECT_PROXY_H
#define __STAMNOS_DPO_COMMON_OBJECT_PROXY_H

#include <pthread.h>
#include "common/list.h"
#include "dpo/common/obj.h"


namespace dpo {

namespace common {

class ObjectProxyReference; // forward reference

// this serves as the base class type
class ObjectProxy {
friend class ObjectProxyReference;
public:
	ObjectProxy()
		: refcnt_(0),
		  subject_(NULL)  
	{
		pthread_mutex_init(&mutex_, NULL);
		INIT_LIST_HEAD(&list_);
	}

	ObjectProxy(dpo::common::ObjectId oid)
		: refcnt_(0)
	{
		pthread_mutex_init(&mutex_, NULL);
		INIT_LIST_HEAD(&list_);
		subject_ = static_cast<dpo::common::Object*>(oid.addr());
	}

	dpo::common::ObjectId oid() {
		return subject_->oid();
	}

protected:
	pthread_mutex_t      mutex_;
	struct list_head     list_;
	int                  refcnt_;
	dpo::common::Object* subject_;
};


class ObjectProxyReference {
public:
	ObjectProxyReference() 
		: obj_(NULL)
	{ }

	ObjectProxy* obj() { 
		return obj_;
	}

	void Set(ObjectProxy* obj, bool lock = true) {
		if (lock) {
			pthread_mutex_lock(&(obj_->mutex_));
		}
		assert(obj_ == NULL);
		obj_ = obj;
		obj_->refcnt_++;
		list_add_tail(&list_, &(obj->list_));
		if (lock) {
			pthread_mutex_unlock(&(obj_->mutex_));
		}
	}

	void Reset(bool lock = true) {
		if (obj_ == NULL) {
			return;
		}
		if (lock) {
			pthread_mutex_lock(&(obj_->mutex_));
		}
		obj_->refcnt_++;
		list_del(&list_);
		if (lock) {
			pthread_mutex_unlock(&(obj_->mutex_));
		}
		obj_ = NULL;
	}

protected:
	ObjectProxy*     obj_;
	struct list_head list_;
};
 
 
} // namespace common
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H
