#include "pxfs/client/inode.h"
#include "common/errno.h"
#include "bcs/bcs.h"
#include "pxfs/client/session.h"

namespace client {

Inode::Inode()
	: ref_(NULL),
	  refcnt_(0),
	  sb_(NULL)
{ 
	pthread_mutex_init(&mutex_, NULL);
}

int 
Inode::Get() 
{ 
	pthread_mutex_lock(&mutex_);
	dbg_log (DBG_INFO, "Inode(%p, oid=%lx)::Get %d -> %d\n", this, this->oid().u64(), refcnt_, refcnt_ + 1);
	refcnt_++; 
	pthread_mutex_unlock(&mutex_);
	return 0; 
}


int 
Inode::Put() 
{ 
	pthread_mutex_lock(&mutex_);
	dbg_log (DBG_INFO, "Inode(%p, oid=%lx)::Put %d -> %d\n", this, this->oid().u64(), refcnt_, refcnt_ - 1);
	assert(refcnt_>0); 
	refcnt_--; 
	pthread_mutex_unlock(&mutex_);
	return 0; 
}

#if 0

int
Inode::Lock(::client::Session* session, lock_protocol::Mode mode)
{
	ssa::cc::client::ObjectProxy* cc_proxy;

	if (ref_) {
		cc_proxy = static_cast<ssa::cc::client::ObjectProxy*>(ref_->obj());	
		return cc_proxy->Lock(session, mode);
	}
	return E_SUCCESS;
}


int
Inode::Lock(::client::Session* session, Inode* parent_inode, lock_protocol::Mode mode)
{
	ssa::cc::client::ObjectProxy* cc_proxy;
	ssa::cc::client::ObjectProxy* cc_proxy_parent;

	if (ref_) {
		cc_proxy = static_cast<ssa::cc::client::ObjectProxy*>(ref_->obj());	
		if (parent_inode->ref_) {
			cc_proxy_parent = static_cast<ssa::cc::client::ObjectProxy*>(parent_inode->ref_->obj());	
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
	ssa::cc::client::ObjectProxy* cc_proxy;

	if (ref_) {
		cc_proxy = static_cast<ssa::cc::client::ObjectProxy*>(ref_->obj());	
		return cc_proxy->Unlock(session);
	}
	return E_SUCCESS;
}

#endif

} // namespace client
