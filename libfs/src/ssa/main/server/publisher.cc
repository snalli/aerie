#include "ssa/main/server/publisher.h"
#include "ssa/main/server/session.h"
#include "ssa/main/server/shbuf.h"
#include "ssa/main/common/publisher.h"
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
Publisher::RegisterOperation(int lgc_op_id, LogicalOperation lgc_op)
{
	if (lgc_op_id < kLogicalOpMaxCount) {
		return -E_INVAL;
	}
	lgc_op_array_[lgc_op_id] = lgc_op;
	return E_SUCCESS;
}



//
// TODO: A logical Operation corresponds to a function defined by the storage system.
// the function reads each command, validates and executes.
//
//
int
Publisher::Publish(SsaSession* session)
{
	char buf[512];
	
	printf("PUBLISH\n");
	SsaSharedBuffer* shbuf = session->shbuf_;
	shbuf->Acquire();
	while (shbuf->Read(buf, 8)) {
		uint64_t u64 = *((uint64_t*) buf);
		printf("LOGICAL OP: %d\n", u64);
		while (shbuf->Read(buf, 8)) {
			ssa::Publisher::Messages::CommandHeader* cmdheader = ssa::Publisher::Messages::CommandHeader::Load(buf);
			printf("COMMAND OP: %d\n", cmdheader->id_);
			if (cmdheader->id_ == 0) {		
				printf("END LOGICAL OP\n");
				break;
			}
			if (cmdheader->id_ == 1) {		
				size_t size = sizeof(ssa::Publisher::Messages::Commands::AllocateExtent);
				if (size > 8) {
					shbuf->Read(&buf[8], size - 8);
					ssa::Publisher::Messages::Commands::AllocateExtent* cmd = ssa::Publisher::Messages::Commands::AllocateExtent::Load(buf);
				}
			}
			if (cmdheader->id_ == 2) {		
				size_t size = sizeof(ssa::Publisher::Messages::Commands::LinkBlock);
				if (size > 8) {
					printf("READREAD\n");
					shbuf->Read(&buf[8], size - 8);
					ssa::Publisher::Messages::Commands::LinkBlock* cmd = ssa::Publisher::Messages::Commands::LinkBlock::Load(buf);
				}
			}
		}
	}
	shbuf->Release();
	return E_SUCCESS;
}


int
Publisher::IpcHandlers::Register(Publisher* publisher)
{
	publisher_ = publisher;
    publisher_->ipc_->reg(::ssa::Publisher::Protocol::kPublish, this, 
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
