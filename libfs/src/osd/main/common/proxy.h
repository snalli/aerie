#ifndef __STAMNOS_OSD_COMMON_OBJECT_PROXY_H
#define __STAMNOS_OSD_COMMON_OBJECT_PROXY_H

#include <pthread.h>
#include <list>
#include "osd/main/common/obj.h"
#include <stdio.h>
#include "pxfs/client/cache.h"

namespace osd {

namespace common {
class ObjectProxy; // forward reference
class ObjectProxyReference {
friend class ObjectProxy;
public:
	pthread_spinlock_t ref_lock;
	int pshared;
	void lock(){
		pthread_spin_lock(&ref_lock);
	}
	
	void unlock(){
		pthread_spin_unlock(&ref_lock);
	}

	ObjectProxyReference(void* owner = NULL) 
		: proxy_(NULL),
		  owner_(owner)
	{ 
		pthread_spin_init(&ref_lock, pshared);
	}

	ObjectProxy* proxy() { 
		return proxy_;
	}

	void* owner() {
		return owner_;
	}

	void set_owner(void* owner) {
		owner_ = owner;
	}

	inline void Set(ObjectProxy* obj, bool lock = true);
	inline void Reset(bool lock = true);
//	~ObjectProxyReference() { printf("\n + Destroying ObjectProxyReference."); }

protected:

	ObjectProxy* proxy_;
	void*        owner_; //!< the volatile object that owns this reference 
	                     //!< (has it embedded in its structure)
};

// this serves as the base class type
class ObjectProxy {
friend class ObjectProxyReference;
protected:
        #ifdef CONFIG_CACHE
	#ifdef CONFIG_CALLBACK
        Cache *cache;
        char self_name[256];
        #endif
        #endif

public:

    #ifdef CONFIG_CACHE
	#ifdef CONFIG_CALLBACK
        void set_name_cache(char *path, osd::common::Cache* val) {
                cache = val;
                strcpy(self_name,path);
  //              printf("\n%016llX %s %s cache %016llX",this, __func__, self_name,cache);

        }
        void invalidate_self_in_cache(bool flush = false){
                if(flush) {
//                printf("\n%016llX %s %s cache %016llX",this, __func__, self_name,cache);
                        if(cache)
                        {
                        cache->flush_cache(self_name);
                        }
                }
        }

	    #endif
    #endif
	
	pthread_spinlock_t proxy_lock;
	int pshared;
	
	void lock(){
		pthread_spin_lock(&(proxy_lock));
	}

	void unlock(){
                pthread_spin_unlock(&(proxy_lock));
        }

        ObjectProxy()
                : refcnt_(0),
                  object_(NULL)
        {
                pthread_mutex_init(&mutex_, NULL);
		pthread_spin_init(&proxy_lock, pshared);
                #ifdef CONFIG_CACHE
			#ifdef CONFIG_CALLBACK
			cache = NULL;
			#endif
		#endif
        }

	ObjectProxy(osd::common::ObjectId oid)
		: refcnt_(0)
	{
		pthread_mutex_init(&mutex_, NULL);
		pthread_spin_init(&proxy_lock, pshared);
//		printf("\n Verify 5. In osd/main/common/proxy.h");
		object_ = static_cast<osd::common::Object*>(oid.addr());
	}

	osd::common::ObjectId oid() {
		return object_->oid();
	}

	osd::common::Object* object() {
		return object_;
	}

	ObjectProxyReference* HeadReference() {
		ObjectProxyReference* ref = NULL;
		pthread_mutex_lock(&mutex_);
		if (refcnt_ > 0) {
			ref = reflist_.front();// insight : Who fills this list ? What if the list is empty ?
		} 
		pthread_mutex_unlock(&mutex_);
		return ref;
	}
//	~ObjectProxy() { printf("\n + Destroying ObjectProxy."); }
protected:

	pthread_mutex_t                  mutex_;
	std::list<ObjectProxyReference*> reflist_;   // list of references to this object proxy 
	int                              refcnt_;
	osd::common::Object*             object_;
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
} // namespace osd

#endif // __STAMNOS_OSD_CLIENT_OBJECT_PROXY_H
