#include "osd/main/server/publisher.h"
#include "osd/main/server/session.h"
#include "osd/main/server/shbuf.h"
#include "osd/main/server/salloc.h"
#include "osd/main/server/stats.h"
#include "osd/main/common/publisher.h"
#include "common/errno.h"
#include "common/util.h"
#include "common/hrtime.h"
#include "bcs/main/common/cdebug.h"
#include "bcs/bcs.h"

namespace osd {
namespace server {

template<typename T>
T* LoadLogicalOperation(::osd::server::OsdSession* session, char* buf)
{
	int                                               n;
	T*                                                lgc_op;
	::osd::server::OsdSharedBuffer*                   shbuf = session->shbuf_;
	osd::Publisher::Message::LogicalOperationHeader* header = osd::Publisher::Message::LogicalOperationHeader::Load(buf);
	n = sizeof(*lgc_op) - sizeof(*header);
	if (shbuf->Read(&buf[sizeof(*header)], n) < n) {
		return NULL;
	}
	lgc_op = T::Load(buf);
	return lgc_op;
}




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
	RegisterOperation(osd::Publisher::Message::LogicalOperation::kAllocContainer, Publisher::AllocContainer);
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
Publisher::Publish(OsdSession* session)
{
	int                                    ret;
	char                                   buf[512];
	int                                    n;
	osd::Publisher::Message::BaseMessage*  msg;
	osd::Publisher::Message::BaseMessage   next;
	LogicalOperation                       lgc_op;
	hrtime_t                               start;
	hrtime_t                               stop;

	dbg_log(DBG_INFO, "PUBLISH\n");
	STATISTICS_INC(publish);

	start = hrtime_cycles();
	OsdSharedBuffer* shbuf = session->shbuf_;
	shbuf->Acquire();
	while (shbuf->Read(buf, sizeof(*msg))) {
		msg = osd::Publisher::Message::BaseMessage::Load(buf);
		while (true) {
			if (msg->type_ == osd::Publisher::Message::kTransactionBegin) {
				n = sizeof(osd::Publisher::Message::TransactionBegin) - sizeof(*msg);
				if (shbuf->Read(&buf[sizeof(*msg)], n) < n) {
					ret = -1;
					goto done;
				}
				session->journal()->TransactionBegin(0);
				break;
			} 
			if (msg->type_ == osd::Publisher::Message::kTransactionCommit) {
				session->journal()->TransactionCommit();
				break;
			} 
			if (msg->type_ == osd::Publisher::Message::kLogicalOperation) {
				n = sizeof(osd::Publisher::Message::LogicalOperationHeader) - sizeof(*msg);
				if (shbuf->Read(&buf[sizeof(*msg)], n) < n) {
					ret = -1;
					goto done;
				}
				osd::Publisher::Message::LogicalOperationHeader* header = osd::Publisher::Message::LogicalOperationHeader::Load(buf);
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
	stop = hrtime_cycles();
	STATISTICS_INC_BY(publish_time, stop - start);
	if (ret != E_SUCCESS) {
		dbg_log(DBG_INFO, "FAILED VALIDATION FOR CLIENT %d\n", session->clt());
	}
	shbuf->Release();
	return ret;
}


int
Publisher::AllocContainer(::osd::server::OsdSession* session, char* buf, 
                          ::osd::Publisher::Message::BaseMessage* next)
{
	int                   ret = E_SUCCESS;
	osd::common::ObjectId set_oid;

	::osd::Publisher::Message::LogicalOperation::AllocContainer* lgc_op = LoadLogicalOperation< ::osd::Publisher::Message::LogicalOperation::AllocContainer>(session, buf);
	
	dbg_log(DBG_INFO, "VALIDATE CONTAINER ALLOCATION: capability=%d, index_hint=%d, oid=%lx\n", 
	        lgc_op->capability_, lgc_op->index_hint_, lgc_op->oid_.u64());
	
	set_oid = session->sets_[lgc_op->capability_];
	
	return session->salloc()->AllocateContainerFromSet(session, set_oid, lgc_op->oid_, lgc_op->index_hint_);
}


int
Publisher::IpcHandlers::Register(Publisher* publisher)
{
	publisher_ = publisher;
    publisher_->ipc_->reg(::osd::Publisher::Protocol::kPublish, this, 
	                      &::osd::server::Publisher::IpcHandlers::Publish);

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
	if ((ret = publisher_->Publish(static_cast<OsdSession*>(session))) < 0) {
		return -ret;
	}
	return E_SUCCESS;
}


} // namespace server
} // namespace osd
