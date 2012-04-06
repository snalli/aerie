#include "ssa/main/server/publisher.h"
#include "ssa/main/server/session.h"
#include "ssa/main/server/shbuf.h"
#include "ssa/main/common/publisher.h"
#include "common/errno.h"
#include "common/util.h"
#include "bcs/main/common/cdebug.h"
#include "bcs/bcs.h"

namespace ssa {
namespace server {

Publisher::Publisher(::server::Ipc* ipc)
	: ipc_(ipc)
{ 
	for (int i=0; i<kLogicalOpMaxCount; i++) {
		lgc_op_array_[i] = NULL;
	}
}


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
	if (lgc_op_id > kLogicalOpMaxCount) {
		return -E_INVAL;
	}
	lgc_op_array_[lgc_op_id] = lgc_op;
	return E_SUCCESS;
}


int
Publisher::Publish(SsaSession* session)
{
	int                                    ret;
	char                                   buf[512];
	int                                    n;
	ssa::Publisher::Messages::BaseMessage* msg;
	ssa::Publisher::Messages::BaseMessage  next;
	LogicalOperation                       lgc_op;
	
	SsaSharedBuffer* shbuf = session->shbuf_;
	shbuf->Acquire();
	while (shbuf->Read(buf, sizeof(*msg))) {
		msg = ssa::Publisher::Messages::BaseMessage::Load(buf);
		while (true) {
			if (msg->type_ == ssa::Publisher::Messages::kTransactionBegin) {
				n = sizeof(ssa::Publisher::Messages::TransactionBegin) - sizeof(*msg);
				if (shbuf->Read(&buf[sizeof(*msg)], n) < n) {
					ret = -1;
					goto done;
				}
				//ssa::Publisher::Messages::TransactionBegin* txbegin = ssa::Publisher::Messages::TransactionBegin::Load(buf);
				break;
			} 
			if (msg->type_ == ssa::Publisher::Messages::kTransactionCommit) {
				printf("DONE\n");
				break;
			} 
			if (msg->type_ == ssa::Publisher::Messages::kLogicalOperation) {
				n = sizeof(ssa::Publisher::Messages::LogicalOperationHeader) - sizeof(*msg);
				if (shbuf->Read(&buf[sizeof(*msg)], n) < n) {
					ret = -1;
					goto done;
				}
				ssa::Publisher::Messages::LogicalOperationHeader* header = ssa::Publisher::Messages::LogicalOperationHeader::Load(buf);
				if ((lgc_op = lgc_op_array_[header->id_]) != NULL) {
					// the buffer buf must have enough space to hold the rest of the logical 
					// operation when lgc_op decodes it
					if ((ret = lgc_op(session, buf, &next)) < 0) {
						goto done;
					}
					// check whether the logical operation handler read one message ahead
					if (ret > 0) {
						msg = &next;
						continue;
					}
					break;
				}
			}
		}
	}
	ret = E_SUCCESS;
done:
	if (ret != E_SUCCESS) {
		dbg_log(DBG_INFO, "FAILED VALIDATION FOR CLIENT %d\n", session->clt());
	}
	shbuf->Release();
	return ret;
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
