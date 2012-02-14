#include "dpo/main/server/stcnr.h"

namespace server {

int
StorageContainer::Create(Session* session)
{
	if ((obj = new(session) dpo::containers::server::StorageContainer::Object) == NULL) {

	}
}


} // namespace server
