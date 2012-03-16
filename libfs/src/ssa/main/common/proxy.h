#ifndef __STAMNOS_SSA_COMMON_OBJECT_PROXY_H
#define __STAMNOS_SSA_COMMON_OBJECT_PROXY_H

#include <pthread.h>
#include <list>
#include "ssa/main/common/obj.h"


namespace ssa {

namespace common {

class ObjectProxy; // forward reference

class ObjectProxyReference {
friend class ObjectProxy;
public:
	ObjectProxyReference(void* owner = NULL) 
		: proxy_(NULL),
		  owner_(owner)
	{ }

	ObjectProxy* proxy() { 
		return proxy_;
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
	ObjectProxy* proxy_;
	void*        owner_; //!< the volatile object that owns this reference 
	                     //!< (has it embedded in its structure)
};


// this serves as the base class type
class ObjectProxy {
friend class ObjectProxyReference;
public:
	ObjectProxy()
		: refcnt_(0),
		  object_(NULL)  
	{
		pthread_mutex_init(&mutex_, NULL);
	}

	ObjectProxy(ssa::common::ObjectId oid)
		: refcnt_(0)
	{
		pthread_mutex_init(&mutex_, NULL);
		object_ = static_cast<ssa::common::Object*>(oid.addr());
	}

	ssa::common::ObjectId oid() {
		return object_->oid();
	}

	ssa::common::Object* object() {
		return object_;
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
	ssa::common::Object*             object_;
};


void ObjectProxyReference::Set(ObjectProxy* proxy, bool lock) {
	if (lock) {
		pthread_mutex_lock(&(proxy->mutex_));
	}
	assert(proxy_ == NULL);
	proxy_ = proxy;
	proxy_->refcnt_++;
	proxy_->reflist_.push_back(this);
	if (lock) {
		pthread_mutex_unlock(&(proxy->mutex_));
	}
}

void ObjectProxyReference::Reset(bool lock) {
	if (proxy_ == NULL) {
		return;
	}
	if (lock) {
		pthread_mutex_lock(&(proxy_->mutex_));
	}
	proxy_->refcnt_++;
	proxy_->reflist_.remove(this);
	if (lock) {
		pthread_mutex_unlock(&(proxy_->mutex_));
	}
	proxy_ = NULL;
}



 
} // namespace common
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_OBJECT_PROXY_H
