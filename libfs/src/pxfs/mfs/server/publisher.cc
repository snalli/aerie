#include "pxfs/mfs/server/publisher.h"
#include "ssa/main/server/stsystem.h"
#include "ssa/main/server/session.h"
#include "ssa/main/server/shbuf.h"
#include "ssa/containers/byte/verifier.h"
#include "common/errno.h"

namespace server {

int
Publisher::Register(::ssa::server::StorageSystem* stsystem)
{
	stsystem->publisher()->RegisterOperation(1, Publisher::Write);
	return E_SUCCESS;
}


template<typename T>
T* LoadLogicalOperation(::ssa::server::SsaSession* session, char* buf)
{
	int                                               n;
	T*                                                lgc_op;
	::ssa::server::SsaSharedBuffer*                   shbuf = session->shbuf_;
	ssa::Publisher::Messages::LogicalOperationHeader* header = ssa::Publisher::Messages::LogicalOperationHeader::Load(buf);
	n = sizeof(*lgc_op) - sizeof(*header);
	if (shbuf->Read(&buf[sizeof(*header)], n) < n) {
		return NULL;
	}
	lgc_op = T::Load(buf);
	return lgc_op;
}


// buffer buf already holds the logical operation header
int
Publisher::Write(::ssa::server::SsaSession* session, char* buf, 
                 ::ssa::Publisher::Messages::BaseMessage* next)
{
	::Publisher::Messages::LogicalOperation::Write* lgc_op = LoadLogicalOperation< ::Publisher::Messages::LogicalOperation::Write>(session, buf);

	return write_verifier_->Parse(session, next);
}


int
Publisher::Init()
{
	write_verifier_ = new WriteVerifier();
	return E_SUCCESS;
}


WriteVerifier* Publisher::write_verifier_ = NULL;

} // namespace server
