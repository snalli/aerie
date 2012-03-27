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


int
Publisher::Write(::ssa::server::SsaSession* session, char* lgc_op_hdr, 
                 ::ssa::Publisher::Messages::BaseMessage* next)
{
	printf("WRITE\n");
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
