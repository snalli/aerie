#include "kvfs/client/table.h"
#include "osd/containers/containers.h"
#include "osd/containers/needle/container.h"
#include "osd/main/common/obj.h"
#include "kvfs/common/publisher.h"
#include "kvfs/client/session.h"


namespace client {


// we don't need to provide any atomicity (thread safety) guarantees 
// as the file system loads the table only once
int
Table::Load(::client::Session* session, 
            ::osd::common::ObjectId oid, 
            ::client::Table** ipp)
{
	int                                ret = E_SUCCESS;
	osd::common::ObjectProxyReference* ref;
	Table*                             tp;

	if ((ret = session->omgr_->FindObject(session, oid, &ref)) == E_SUCCESS) {
		if (ref->owner()) {
			// the in-core inode already exists; just return this and 
			// we are done
			tp = reinterpret_cast<Table*>(ref->owner());
		} else {
			tp = new Table(ref);
			ref->set_owner(tp);
		}
	} else {
		tp = new Table(ref);
	}
	*ipp = tp;
	return E_SUCCESS;
}


int
Table::Lock(::client::Session* session, lock_protocol::Mode mode)
{
	osd::containers::client::NameContainer::Proxy* cc_proxy;

	if (rw_ref_) {
		cc_proxy = rw_ref()->proxy();	
		return cc_proxy->Lock(session, mode);
	}

	return E_SUCCESS;
}


int
Table::Unlock(::client::Session* session)
{
	osd::containers::client::NameContainer::Proxy* cc_proxy;

	if (rw_ref_) {
		cc_proxy = rw_ref()->proxy();	
		return cc_proxy->Unlock(session);
	}
	return E_SUCCESS;
}
 

int 
Table::Put(::client::Session* session, const char* key, const char* src, uint64_t n)
{
	int                                               ret;
	osd::common::ObjectId                             oid;
	osd::containers::client::NeedleContainer::Object* obj;
	
	// no metadata for needle container so don't use the proxy but write 
	// directly via the object
	Lock(session, lock_protocol::Mode::XL);
	if ((ret = rw_ref()->proxy()->interface()->Find(session, key, &oid)) != E_SUCCESS) {
		if ((obj = osd::containers::client::NeedleContainer::Object::Make(session)) == NULL) {
			ret = -E_NOMEM;
			goto done;
		}
		if ((ret = rw_ref()->proxy()->interface()->Insert(session, key, obj->oid())) < 0) {
			goto done;
		}
		session->journal()->TransactionBegin();
		session->journal() << Publisher::Message::LogicalOperation::MakeFile(rw_ref()->proxy()->oid().u64(), key, obj->oid().u64());
		session->journal()->TransactionCommit();
	} else {
		if ((obj = osd::containers::client::NeedleContainer::Object::Load(oid)) == NULL) {
			ret = -E_NOMEM;
			goto done;
		}
	}
	if ((ret = obj->Write(session, src, 0, n)) != n) {
		ret = -E_NOMEM;
		goto done;
	}

done:
	Unlock(session);
	return ret;
}


int 
Table::Get(::client::Session* session, const char* key, char* dst)
{
	int                                               ret;
	osd::common::ObjectId                             oid;
	osd::containers::client::NeedleContainer::Object* obj;
	
	Lock(session, lock_protocol::Mode::SL);
	// no metadata for needle container so don't use the proxy. Write
	// directly via the object instead.
	if ((ret = rw_ref()->proxy()->interface()->Find(session, key, &oid)) != E_SUCCESS) {
		ret = -E_NOENT;
		goto done;
	} else {
		if ((obj = osd::containers::client::NeedleContainer::Object::Load(oid)) == NULL) {
			ret = -E_NOMEM;
			goto done;
		}
	}
	if ((ret = obj->Read(session, dst, 0, obj->Size())) < 0) {
		ret = -E_NOMEM;
		goto done;
	}

done:
	Unlock(session);
	return ret;
}


} // namespace client
