#include "kvfs/client/sb.h"
#include "kvfs/client/session.h"
#include "kvfs/client/table.h"
#include "osd/main/common/obj.h"
#include "osd/containers/super/container.h"
#include "osd/containers/name/container.h"


namespace client {

int 
SuperBlock::Load(::client::Session* session, osd::common::ObjectId oid, 
                 ::client::SuperBlock** sbp)
{
	int                                ret;
	SuperBlock*                        sb; 
	::client::Table*                   tp;
	osd::common::ObjectProxyReference* ref;
	osd::common::ObjectId              table_oid;

	if ((ret = session->omgr_->FindObject(session, oid, &ref)) == E_SUCCESS) {
		if (ref->owner()) {
			// the in-core superblock already exists; just return this and 
			// we are done
			sb = reinterpret_cast<SuperBlock*>(ref->owner());
			goto done;
		} else {
			sb = new SuperBlock(ref);
		}
	} else {
		sb = new SuperBlock(ref);
	}

	table_oid = sb->super_rw_ref_->proxy()->interface()->root(session);
	if ((ret = Table::Load(session, table_oid, &tp)) < 0) {
		//FIXME: deallocate the allocated superblock and dirinode object
		return ret;
	}
	sb->tp_ = tp;
done:	
	*sbp = sb;
	return E_SUCCESS;
}

} // namespace client
