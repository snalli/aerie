#ifndef __STAMNOS_DPO_COMMON_OBJECT_PROXY_H
#define __STAMNOS_DPO_COMMON_OBJECT_PROXY_H

#include <pthread.h>
#include <list>
#include "dpo/base/common/obj.h"


namespace dpo {

namespace common {

class ObjectProxy; // forward reference


class ObjectProxyReference {
friend class ObjectProxy;
public:
	ObjectProxyReference(void* owner = NULL) 
		: obj_(NULL),
		  owner_(owner)
	{ }

	ObjectProxy* obj() { 
		return obj_;
	}

	void* owner() {
		return owner_;
	}

	void* set_owner(void* owner) {
		owner_ = owner;
	}

	inline void Set(ObjectProxy* obj, bool lock = true);
	inline void Reset(bool lock = true);

protected:
	ObjectProxy* obj_;
	void*        owner_; // the volatile object that owns this reference 
	                     // (has it embedded in its structure)
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
	}

	ObjectProxy(dpo::common::ObjectId oid)
		: refcnt_(0)
	{
		pthread_mutex_init(&mutex_, NULL);
		subject_ = static_cast<dpo::common::Object*>(oid.addr());
	}

	dpo::common::ObjectId oid() {
		return subject_->oid();
	}

	ObjectProxyReference* HeadReference() {
		ObjectProxyReference* ref = NULL;
		pthread_mutex_lock(&mutex_);
		if (refcnt_ > 0) {
			ref = reflist_.front();
		} 
		pthread_mutex_unlock(&mutex_);
		return ref;
	}

protected:
	pthread_mutex_t                  mutex_;
	std::list<ObjectProxyReference*> reflist_;   // list of references to this object proxy 
	int                              refcnt_;
	dpo::common::Object*             subject_;
};


void ObjectProxyReference::Set(ObjectProxy* obj, bool lock) {
	if (lock) {
		pthread_mutex_lock(&(obj->mutex_));
	}
	assert(obj_ == NULL);
	obj_ = obj;
	obj_->refcnt_++;
	obj_->reflist_.push_back(this);
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
	obj_->reflist_.remove(this);
	if (lock) {
		pthread_mutex_unlock(&(obj_->mutex_));
	}
	obj_ = NULL;
}



 
} // namespace common
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H
