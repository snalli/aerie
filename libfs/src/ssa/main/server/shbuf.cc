#include "ssa/main/server/shbuf.h"
#include "bcs/main/server/session.h"
#include "ssa/main/server/publisher.h"
#include "ssa/main/server/session.h"
#include "ssa/main/server/stsystem.h"
#include <stdio.h>

namespace ssa {
namespace server {

int SsaSharedBuffer::Consume(::server::BcsSession* session)
{
	SsaSession* ssa_session = static_cast<SsaSession*>(session);
	return ssa_session->storage_system_->publisher()->Publish(ssa_session);
}

} // namespace server
} // namespace ssa
