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
SuperContainer::VersionManager::vUpdate(::client::Session* session)
{
	return 0;
}


dpo::common::ObjectId 
SuperContainer::VersionManager::root(::client::Session* session)
{
	return object()->root(session);
}


int 
SuperContainer::VersionManager::set_root(::client::Session* session, dpo::common::ObjectId oid)
{
	return object()->set_root(session, oid);
}



} // namespace dpo
} // namespace containers
} // namespace client
