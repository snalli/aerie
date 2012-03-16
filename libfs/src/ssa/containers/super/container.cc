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
SuperContainer::VersionManager::vUpdate(::client::Session* session)
{
	return 0;
}


ssa::common::ObjectId 
SuperContainer::VersionManager::root(::client::Session* session)
{
	return object()->root(session);
}


int 
SuperContainer::VersionManager::set_root(::client::Session* session, ssa::common::ObjectId oid)
{
	return object()->set_root(session, oid);
}



} // namespace ssa
} // namespace containers
} // namespace client
