#define  __CACHE_GUARD__

#include "osd/containers/needle/container.h"
#include <stdint.h>
#include <inttypes.h>
#include <vector>
#include "common/errno.h"
#include "osd/main/client/session.h"
#include "osd/main/client/salloc.h"
#include "osd/main/common/const.h"
#include "osd/main/common/publisher.h"
#include "scm/scm.h"

namespace osd {
namespace containers {
namespace client {


int 
NeedleContainer::VersionManager::vOpen()
{
	osd::vm::client::VersionManager<NeedleContainer::Object>::vOpen();
	size_ = object()->Size();

	return E_SUCCESS;
}


// obsolete
int 
NeedleContainer::VersionManager::vUpdate(OsdSession* session)
{
	osd::vm::client::VersionManager<NeedleContainer::Object>::vUpdate(session);
	
	return 0;
}


int 
NeedleContainer::VersionManager::Read(OsdSession* session, char* dst, 
                                    uint64_t off, uint64_t n)
{
	dbg_log (DBG_CRITICAL, "Don't use this. Remember it's a needle. You should go directly through the persistent interface.\n");
	return 0;
}


int 
NeedleContainer::VersionManager::Write(OsdSession* session, 
                                     char* src, 
                                     uint64_t off, 
                                     uint64_t n)
{
	dbg_log (DBG_CRITICAL, "Don't use this. Remember it's a needle. You should go directly through the persistent interface.\n");
	return 0;
}


int
NeedleContainer::VersionManager::Size(OsdSession* session)
{
	return size_;
}

} // namespace osd
} // namespace containers
} // namespace client

