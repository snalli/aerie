#ifndef __STAMNOS_OSD_CLIENT_OBJECT_PROXY_H
#define __STAMNOS_OSD_CLIENT_OBJECT_PROXY_H

#include "osd/main/client/hlckmgr.h"
#include "common/errno.h"
#include "osd/containers/containers.h"
#include "osd/main/common/proxy.h"
#include "osd/main/client/stm.h"
#include "osd/main/client/session.h"
#include "osd/main/common/publisher.h"
#include <stdio.h>
//TODO: Currently we rely on the hierarchical lock manager to transition the lock 
//between hierarchical mode or flat mode (capability). However this complicates the
//lock manager a lot. We could simplify the lock manager by having two separate locks:
//a flat lock and a hierarchical lock, and extent the proxy manager with logic to 
//choose between the two locks. The lock manager though will have to provide the 
//ability to users to couple the locks so that a callback on one of the locks
//triggers a callback to the other as well. For example, trying to acquire the 
//flat lock should revoke the hierarchical lock.


namespace osd {

namespace client {

typedef osd::common::ObjectProxy ObjectProxy;
typedef osd::common::ObjectId    ObjectId; 
typedef osd::common::ObjectType  ObjectType;

} // namespace client


// CONCURRENCY CONTROL

namespace cc {
namespace client {

typedef ::osd::client::OsdSession OsdSession;

class ObjectProxy: public osd::common::ObjectProxy {
// insight : Notice that the name of the child and parent class are same simply because they are in different namespaces
public:
	ObjectProxy(OsdSession* session, osd::common::ObjectId oid) 
		: osd::common::ObjectProxy(oid)
	{ 
		hlock_ = session->hlckmgr_->FindOrCreateLock(LockId(1, oid.num()));
		hlock_->set_payload(reinterpret_cast<void*>(oid.u64()));
	}

	int Lock(OsdSession* session, lock_protocol::Mode mode) {
		s_log("[%ld] ObjectProxy::%s (1)", s_tid, __func__);
		return session->hlckmgr_->Acquire(hlock_, mode, 0);
	}

	//FIXME: it's possible to see dlink == 0 but after grabbing the lock the dlink
	//to be 1 if some other client was forced to revoke the lock but wanted to stick
	//to the lock. In such a case, release the lock you just acquired and grab the 
	//explicit lock
	int Lock(OsdSession* session, osd::cc::client::ObjectProxy* parent, lock_protocol::Mode mode, int flags) {
		s_log("[%ld] ObjectProxy::%s (2)", s_tid, __func__);
		if (object()->dlink() > 0) {
			return session->hlckmgr_->Acquire(hlock_, mode, flags);
		} else {
			assert(parent->hlock_);
			return session->hlckmgr_->Acquire(hlock_, parent->hlock_, mode, flags);
		}
	}

	int Unlock(OsdSession* session) {
		s_log("[%ld] ObjectProxy::%s ", s_tid, __func__);
		return session->hlckmgr_->Release(hlock_);
	}

	// deprecated: this lock certificate contains just a chain of locks. 
	// verifying the lock chain at the server side requires looking 
	// up whether a node is the child of another node, which is 
	// computationally expensive if we don't provide a name or another hint
	// to help the lookup. Relying on the name though is tricky. For example, 
	// if there is a rename, the certificate cannot be validated.
	void JournalLockCertificate(OsdSession* session) {
		char*                                                     buf[128];
		osd::cc::common::LockId                                   lckarray[16];
		int                                                       nlocks = session->hlckmgr_->LockChain(hlock_, lckarray);
		Publisher::Message::ContainerOperation::LockCertificate* certificate = new(buf) Publisher::Message::ContainerOperation::LockCertificate(nlocks);

		assert(sizeof(*certificate) + certificate->payload_size_ < 128); // otherwise we write outside buffer buf
		if (nlocks > 0) {
			for (int i=0; i<nlocks; i++) {
				certificate->locks_[i] = lckarray[nlocks - i - 1];
			}
			session->journal() << *certificate;
		}
	}
//	~ObjectProxy() { printf("\n + Destroying ObjectProxy 001. in src/osd/main/client/proxy.h");}
//	   void printme() { printf("\n* Hailing from ObjectProxy."); }

private:
	osd::cc::client::HLock* hlock_; // hierarchical lock
	osd::cc::client::Lock*  lock_;  // flat lock 
};


template<class Derived, class Subject>
class ObjectProxyTemplate: public ObjectProxy {
public:
	ObjectProxyTemplate(OsdSession* session, osd::common::ObjectId oid)
		: ObjectProxy(session, oid)
	{ //printf("\nVerify 4. In osd/main/client/proxy.h");}
	}

	Subject* object() { return static_cast<Subject*>(object_); }

	Derived* xOpenRO(osd::stm::client::Transaction* tx);
	Derived* xOpenRO();
// ~ObjectProxyTemplate() { printf("\n + Destroying ObjectProxy 002. in src/osd/main/client/proxy.h");}

private:
	osd::stm::client::Transaction* tx_;
};


template<class Derived, class Subject>
Derived* ObjectProxyTemplate<Derived, Subject>::xOpenRO(osd::stm::client::Transaction* tx)
{
	Derived* derived = static_cast<Derived*>(this);
	Subject* subj = derived->object();
	tx->OpenRO(subj);
	return derived;
}


template<class Derived, class Subject>
Derived* ObjectProxyTemplate<Derived, Subject>::xOpenRO()
{
	osd::stm::client::Transaction* tx = osd::stm::client::Self();
	return xOpenRO(tx);
}


} // namespace client
} // namespace cc


// VERSION MANAGEMENT

namespace vm {
namespace client {

typedef ::osd::client::OsdSession OsdSession;

// This class is inherited by the VersionManager class defined by each object. 
// It must provide the same interface as the underlying object
template<class Subject>
class VersionManager {
public:
	VersionManager(Subject* object = NULL)
		: object_(object),
		  nlink_(0)
	{ //printf("\n-------> Ola Mofus !");}
	}

	void set_object(Subject* object) {
		object_ = object;
	}
	
	Subject* object() {
		return object_;
	}

	int vOpen() {
		nlink_ = object_->nlink();
		return 0;
	}
	
	int vUpdate(OsdSession* session) {
		//FIXME: version counter must be updated by the server
		//printf("\n Chkpt 1 : Inside VersionManager::vUpdate");	
		object_->ccSetVersion(object_->ccVersion() + 1);
		return 0;
	}

	int nlink() {
		return nlink_;
	}

	int set_nlink(int nlink) {
		nlink_ = nlink;
		return 0;
	}

protected:
	Subject* object_;
	int      nlink_;
};


// FIXME: Currently we rely on composition to parameterize 
// ObjectProxy on the VersionManager. 
// Can we make VersionManager a mixin layer instead? This would 
// save us from the extra object_ kept in the VersionManager class
// and avoid the call to the ugly interface() to access the shadow object

//insight : namespace : osd::vm::client
template<class Derived, class Subject, class VersionManager>
class ObjectProxy: public osd::cc::client::ObjectProxyTemplate<Derived, Subject>
{
public:
	ObjectProxy(OsdSession* session, osd::common::ObjectId oid)
		: osd::cc::client::ObjectProxyTemplate<Derived, Subject>(session, oid),
		  valid_(false)
	{ 
		// CAREFUL:
		// We don't initialize in the initialization list as it is risky. We
		// must be sure that 
		// osd::cc::client::ObjectProxyTemplate<Derived, Subject>
		// has been initialized first.
		//printf("\nVerifying 3. In osd/main/client/proxy.h"); 
		vm_.set_object(osd::cc::client::ObjectProxyTemplate<Derived, Subject>::object());
	}

	// provides access to the buffered state
	VersionManager* interface() {
		//printf("\nInside osd::vm::client::ObjectProxy::interface().");
		return &vm_;
	}

	// for accessing the object when using the RO-STM mechanism
	// provides direct access to the underlying object. it bypasses any buffering
	// done by the copy-on-right mechanism.
	Subject* xinterface() {
		return osd::cc::client::ObjectProxyTemplate<Derived, Subject>::object();
	}

	int vOpen() {
		int ret;
		if (!valid_) {
			if ((ret = vm_.vOpen()) < 0) {
				return ret;
			}
			valid_ = true;
		}
		return 0;
	}
	
	int vUpdate(OsdSession* session) {
		return (valid_ ? vm_.vUpdate(session): E_INVAL);
	}

	/**
	 * FIXME:RACE
	 * Currently it's possible we lose a race and close a locked object.
	 * Here is a description of the problem and how we could solve it:
	 * 
	 * Invoking vClose with update==false without holding the lock while someone 
	 * has the object locked must have no effect. The object's COW image should 
	 * stay unaffected.
	 * This is fine as the object can continue reading it's updates through its
	 * COW image even if it's updates are applied to the public image. Otherwise,
	 * we would have to acquire the lock before closing the object. But then we
	 * have to care about deadlock prevention. We could rely on a latch (mutex) 
	 * that protects the COW image and acquire that one instead of the lock 
	 * when closing the object. This however would add extra overhead on the common
	 * path. We could use biased mutexes to avoid the overhead of locking on the 
	 * common case.
	 */
	int vClose(OsdSession* session, bool update, bool flush = false) 
	{
		int ret = E_SUCCESS;
		
		#ifdef CONFIG_CACHE
		#ifdef CONFIG_CALLBACK
		osd::common::ObjectProxy::invalidate_self_in_cache(flush);
		#endif
		#endif

		if (valid_ && update) {
			ret = vm_.vUpdate(session);
		}
		valid_ = false;
	//	printf("\n + Sanketh : Closing ObjectProxy %s",self_name);
		return ret;
	}
// ~ObjectProxy() { printf("\n + Sanketh : Destroying ObjectProxy 003. in src/osd/main/client/proxy.h");}

protected:
	bool           valid_; // invalid, opened (valid)
	VersionManager vm_;
};


} // namespace client
} // namespace vm

} // namespace osd

#endif // __STAMNOS_OSD_CLIENT_OBJECT_PROXY_H
