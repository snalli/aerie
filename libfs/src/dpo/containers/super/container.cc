#include "dpo/containers/super/container.h"
#include <stdint.h>
#include "common/errno.h"
#include "client/session.h"
#include "client/smgr.h"

namespace dpo {
namespace containers {
namespace client {

int 
SuperContainer::VersionManager::vOpen()
{
	return 0;
}


int 
SuperContainer::VersionManager::vUpdate()
{
	return 0;
}


dpo::common::ObjectId 
SuperContainer::VersionManager::root(Session* session)
{
	return subject()->root();
}


int 
SuperContainer::VersionManager::set_root(Session* session, dpo::common::ObjectId oid)
{
	return subject()->set_root(session, oid);
}



} // namespace dpo
} // namespace containers
} // namespace client
