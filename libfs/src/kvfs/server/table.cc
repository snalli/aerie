#include "kvfs/server/table.h"
#include "kvfs/server/file.h"

namespace server {

int Table::Insert(Session* session, const char* key, File* fp)
{
	obj_->Insert(session, key, fp->oid());
	return E_SUCCESS;
}

int Table::Unlink(Session* session, const char* key)
{
	int                   ret;
	osd::common::ObjectId needle_oid;
	
	if ((ret = obj_->Find(session, key, &needle_oid)) < 0) { return ret; }
	obj_->Erase(session, key);
	osd::containers::server::NeedleContainer::Object::Free(session, needle_oid);
	return E_SUCCESS;
}


} // namespace server
