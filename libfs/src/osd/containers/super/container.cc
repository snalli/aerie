#define __CACHE_GUARD__

#include "osd/containers/super/container.h"
#include "common/errno.h"
#include "osd/main/client/salloc.h"
#include "osd/main/client/session.h"
#include <stdint.h>

namespace osd
{
namespace containers
{
namespace client
{

int SuperContainer::VersionManager::vOpen()
{
    return 0;
}

int SuperContainer::VersionManager::vUpdate(OsdSession* /*session*/)
{
    return 0;
}

osd::common::ObjectId SuperContainer::VersionManager::root(OsdSession* session)
{
    return object()->root(session);
}

void SuperContainer::VersionManager::set_root(OsdSession* session, osd::common::ObjectId oid)
{
    object()->set_root(session, oid);
}

} // namespace client
} // namespace containers
} // namespace osd
