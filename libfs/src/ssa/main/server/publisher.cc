#include "ssa/main/server/publisher.h"
#include "ssa/main/server/session.h"
#include "ssa/main/common/publisher_protocol.h"
#include "common/errno.h"
#include "common/util.h"
#include "bcs/bcs.h"

namespace ssa {
namespace server {

Publisher::Publisher(::server::Ipc* ipc)
	: ipc_(ipc)
{ }

int
Publisher::Init()
{
	if (ipc_) {
		return ipc_handlers_.Register(this);
	}
	return E_SUCCESS;
}


int
Publisher::Publish(SsaSession* session)
{
	return E_SUCCESS;
}


int
Publisher::IpcHandlers::Register(Publisher* publisher)
{
	publisher_ = publisher;
    publisher_->ipc_->reg(::ssa::PublisherProtocol::kPublish, this, 
	                      &::ssa::server::Publisher::IpcHandlers::Publish);

	return E_SUCCESS;
}


int
Publisher::IpcHandlers::Publish(int clt, int& unused)
{
	int                    ret;
	::server::BaseSession* session;
	
	if ((ret = publisher_->ipc_->session_manager()->Lookup(clt, &session)) < 0) {
		return -ret;
	}
	if ((ret = publisher_->Publish(static_cast<SsaSession*>(session))) < 0) {
		return -ret;
	}
	return E_SUCCESS;
}


} // namespace server
} // namespace ssa
