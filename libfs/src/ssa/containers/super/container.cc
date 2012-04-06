#include "ssa/containers/super/container.h"
#include <stdint.h>
#include "common/errno.h"
#include "ssa/main/client/session.h"
#include "ssa/main/client/salloc.h"

namespace ssa {
namespace containers {
namespace client {

int 
SuperContainer::VersionManager::vOpen()
{
	return 0;
}


int 
SuperContainer::VersionManager::vUpdate(SsaSession* session)
{
	return 0;
}


ssa::common::ObjectId 
SuperContainer::VersionManager::root(SsaSession* session)
{
	return object()->root(session);
}


void 
SuperContainer::VersionManager::set_root(SsaSession* session, ssa::common::ObjectId oid)
{
	object()->set_root(session, oid);
}



} // namespace ssa
} // namespace containers
} // namespace client
