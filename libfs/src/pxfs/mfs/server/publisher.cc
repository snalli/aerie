#include "pxfs/mfs/server/publisher.h"
#include "ssa/main/server/stsystem.h"
#include "ssa/main/server/session.h"
#include "ssa/main/server/shbuf.h"
#include "ssa/main/server/verifier.h"
#include "ssa/containers/byte/verifier.h"
#include "pxfs/server/session.h"
#include "pxfs/mfs/server/dir_inode.h"
#include "common/errno.h"

namespace server {

int
Publisher::Register(::ssa::server::StorageSystem* stsystem)
{
	stsystem->publisher()->RegisterOperation(::Publisher::Messages::LogicalOperation::kLink, Publisher::Link);
	stsystem->publisher()->RegisterOperation(::Publisher::Messages::LogicalOperation::kUnlink, Publisher::Unlink);
	stsystem->publisher()->RegisterOperation(::Publisher::Messages::LogicalOperation::kWrite, Publisher::Write);
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


int
Publisher::Link(::ssa::server::SsaSession* session, char* buf, 
                ::ssa::Publisher::Messages::BaseMessage* next)
{
	int                   ret;
	ssa::common::ObjectId oid;
	DirInode              dinode;
	
	::Publisher::Messages::LogicalOperation::Link* lgc_op = LoadLogicalOperation< ::Publisher::Messages::LogicalOperation::Link>(session, buf);
	
	// verify preconditions
	oid = ssa::common::ObjectId(lgc_op->parino_);
	if ((ret = lock_verifier_->VerifyLock(session, oid)) < 0) {
		return ret;
	}

	// do the operation 
	if (Inode::type(lgc_op->parino_) != kDirInode) {
		return -1;
	}
	DirInode* dp = DirInode::Make(&dinode, lgc_op->parino_);
	return dp->Link(static_cast<Session*>(session), lgc_op->name_, lgc_op->childino_);
}


int
Publisher::Unlink(::ssa::server::SsaSession* session, char* buf, 
                  ::ssa::Publisher::Messages::BaseMessage* next)
{
	int                   ret;
	ssa::common::ObjectId oid;
	
	::Publisher::Messages::LogicalOperation::Unlink* lgc_op = LoadLogicalOperation< ::Publisher::Messages::LogicalOperation::Unlink>(session, buf);
	
	// verify preconditions
	oid = ssa::common::ObjectId(lgc_op->parino_);
	if ((ret = lock_verifier_->VerifyLock(session, oid)) < 0) {
		return ret;
	}

	// do the operation 
	

	return E_SUCCESS;
}



// buffer buf already holds the logical operation header
int
Publisher::Write(::ssa::server::SsaSession* session, char* buf, 
                 ::ssa::Publisher::Messages::BaseMessage* next)
{
	int                   ret;
	ssa::common::ObjectId oid;

	::Publisher::Messages::LogicalOperation::Write* lgc_op = LoadLogicalOperation< ::Publisher::Messages::LogicalOperation::Write>(session, buf);
	
	// verify preconditions
	oid = ssa::common::ObjectId(lgc_op->ino_);
	if ((ret = lock_verifier_->VerifyLock(session, oid)) < 0) {
		return ret;
	}
	// verify each container operation and then apply it
	return write_verifier_->Parse(session, next);
}


int
Publisher::Init()
{
	lock_verifier_ = new LockVerifier();
	write_verifier_ = new WriteVerifier();
	return E_SUCCESS;
}


WriteVerifier* Publisher::write_verifier_ = NULL;
LockVerifier*  Publisher::lock_verifier_ = NULL;

} // namespace server
