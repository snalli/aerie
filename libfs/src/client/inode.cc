#include "client/inode.h"
#include "common/errno.h"
#include "client/session.h"

namespace client {

Inode::Inode()
	: sb_(NULL),
	  pnode_(NULL),
	  refcnt_(0),
	  ref_(NULL)
{ 
	printf("Inode::Inode: %p\n", this);
	pthread_mutex_init(&mutex_, NULL);
}


int
Inode::Lock(::client::Session* session, lock_protocol::Mode mode)
{
	dpo::cc::client::ObjectProxy* cc_proxy;

	if (ref_) {
		cc_proxy = static_cast<dpo::cc::client::ObjectProxy*>(ref_->obj());	
		return cc_proxy->Lock(session, mode);
	}
	return E_SUCCESS;
}


int
Inode::Lock(::client::Session* session, Inode* parent_inode, lock_protocol::Mode mode)
{
	dpo::cc::client::ObjectProxy* cc_proxy;
	dpo::cc::client::ObjectProxy* cc_proxy_parent;

	if (ref_) {
		cc_proxy = static_cast<dpo::cc::client::ObjectProxy*>(ref_->obj());	
		if (parent_inode->ref_) {
			cc_proxy_parent = static_cast<dpo::cc::client::ObjectProxy*>(parent_inode->ref_->obj());	
			return cc_proxy->Lock(session, cc_proxy_parent, mode);
		} else {
			return cc_proxy->Lock(session, mode);
		}
	}

	return E_SUCCESS;
}


int
Inode::Unlock(::client::Session* session)
{
	dpo::cc::client::ObjectProxy* cc_proxy;

	if (ref_) {
		cc_proxy = static_cast<dpo::cc::client::ObjectProxy*>(ref_->obj());	
		return cc_proxy->Unlock(session);
	}
	return E_SUCCESS;
}


int 
Inode::Get() 
{ 
	pthread_mutex_lock(&mutex_);
	printf("Inode(%p)::Get %d -> %d\n", this, refcnt_, refcnt_ + 1);
	refcnt_++; 
	pthread_mutex_unlock(&mutex_);
	return 0; 
}


int 
Inode::Put() 
{ 
	pthread_mutex_lock(&mutex_);
	printf("Inode(%p)::Put %d -> %d\n", this, refcnt_, refcnt_ - 1);
	assert(refcnt_>0); 
	refcnt_--; 
	pthread_mutex_unlock(&mutex_);
	return 0; 
}

} // namespace client
