#include "osd/main/server/shbuf.h"
#include "bcs/main/server/session.h"
#include "osd/main/server/publisher.h"
#include "osd/main/server/session.h"
#include "osd/main/server/stsystem.h"
#include <stdio.h>

namespace osd {
namespace server {

int OsdSharedBuffer::Consume(::server::BcsSession* session)
{
	OsdSession* osd_session = static_cast<OsdSession*>(session);
	return osd_session->storage_system_->publisher()->Publish(osd_session);
}

} // namespace server
} // namespace osd
