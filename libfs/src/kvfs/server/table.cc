#include "kvfs/server/table.h"
#include "kvfs/server/file.h"

namespace server {

int Table::Insert(Session* session, const char* key, File* fp)
{
	obj_->Insert(session, key, fp->oid());
	return E_SUCCESS;
}


} // namespace server
