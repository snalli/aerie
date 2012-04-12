#include "kvfs/server/publisher.h"
#include "osd/main/server/stsystem.h"
#include "osd/main/server/session.h"
#include "osd/main/server/shbuf.h"
#include "osd/main/server/verifier.h"
#include "osd/containers/byte/verifier.h"
#include "kvfs/server/session.h"
//#include "kvfs/mfs/server/file_inode.h"
#include "common/errno.h"

namespace server {

int
Publisher::Register(::osd::server::StorageSystem* stsystem)
{
	stsystem->publisher()->RegisterOperation(::Publisher::Message::LogicalOperation::kMakeFile, Publisher::MakeFile);
	stsystem->publisher()->RegisterOperation(::Publisher::Message::LogicalOperation::kUnlink, Publisher::Unlink);
	return E_SUCCESS;
}


template<typename T>
T* LoadLogicalOperation(::osd::server::OsdSession* session, char* buf)
{
	int                                              n;
	T*                                               lgc_op;
	::osd::server::OsdSharedBuffer*                  shbuf = session->shbuf_;
	osd::Publisher::Message::LogicalOperationHeader* header = osd::Publisher::Message::LogicalOperationHeader::Load(buf);
	n = sizeof(*lgc_op) - sizeof(*header);
	if (shbuf->Read(&buf[sizeof(*header)], n) < n) {
		return NULL;
	}
	lgc_op = T::Load(buf);
	return lgc_op;
}


int
Publisher::MakeFile(::osd::server::OsdSession* osdsession, char* buf, 
                    ::osd::Publisher::Message::BaseMessage* next)
{
	int                   ret;
	osd::common::ObjectId oid;
	//DirInode              dinode;
	//FileInode             finode;
	Session*              session = static_cast<Session*>(osdsession);
	
	::Publisher::Message::LogicalOperation::MakeFile* lgc_op = LoadLogicalOperation< ::Publisher::Message::LogicalOperation::MakeFile>(session, buf);
	
	// verify preconditions
	oid = osd::common::ObjectId(lgc_op->parino_);
	if ((ret = lock_verifier_->VerifyLock(session, oid)) < 0) {
		return ret;
	}

	/*
	// do the operation 
	if (Inode::type(lgc_op->parino_) != kDirInode) {
		return -1;
	}
	DirInode* pp = DirInode::Load(session, lgc_op->parino_, &dinode);
	FileInode* cp = FileInode::Make(session, lgc_op->childino_, &finode);
	pp->Link(session, lgc_op->name_, cp);
	*/

	return E_SUCCESS;
}


int
Publisher::Unlink(::osd::server::OsdSession* osdsession, char* buf, 
                  ::osd::Publisher::Message::BaseMessage* next)
{
	int                   ret;
	osd::common::ObjectId oid;
	//DirInode              dinode;
	//InodeNumber           child_ino;
	Session*              session = static_cast<Session*>(osdsession);
	
	::Publisher::Message::LogicalOperation::Unlink* lgc_op = LoadLogicalOperation< ::Publisher::Message::LogicalOperation::Unlink>(session, buf);
	
	// verify preconditions
	oid = osd::common::ObjectId(lgc_op->parino_);
	if ((ret = lock_verifier_->VerifyLock(session, oid)) < 0) {
		return ret;
	}

	// do the operation 
	//DirInode* dp = DirInode::Load(session, lgc_op->parino_, &dinode);
	//return dp->Unlink(session, lgc_op->name_);
	return E_SUCCESS;
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
