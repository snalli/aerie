#include "kvfs/server/publisher.h"
#include "osd/main/server/stsystem.h"
#include "osd/main/server/session.h"
#include "osd/main/server/shbuf.h"
#include "osd/main/server/verifier.h"
#include "osd/containers/byte/verifier.h"
#include "kvfs/server/session.h"
#include "kvfs/server/file.h"
#include "kvfs/server/table.h"
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
	Table                 table;
	File                  file;
	Session*              session = static_cast<Session*>(osdsession);
	
	::Publisher::Message::LogicalOperation::MakeFile* lgc_op = LoadLogicalOperation< ::Publisher::Message::LogicalOperation::MakeFile>(session, buf);
	
	dbg_log (DBG_INFO, "Validate MakeFile: %p %s -> %p\n", lgc_op->parino_, lgc_op->key_, lgc_op->childino_);
	// verify preconditions
	oid = osd::common::ObjectId(lgc_op->parino_);
	if ((ret = lock_verifier_->VerifyLock(session, oid)) < 0) {
		return ret;
	}

	// do the operation 
	Table* tp  = Table::Load(session, lgc_op->parino_, &table);
	File* fp = File::Make(session, lgc_op->childino_, &file);
	tp->Insert(session, lgc_op->key_, fp);

	return E_SUCCESS;
}


int
Publisher::Unlink(::osd::server::OsdSession* osdsession, char* buf, 
                  ::osd::Publisher::Message::BaseMessage* next)
{
	int                   ret;
	osd::common::ObjectId oid;
	Table                 table;
	Session*              session = static_cast<Session*>(osdsession);
	
	::Publisher::Message::LogicalOperation::Unlink* lgc_op = LoadLogicalOperation< ::Publisher::Message::LogicalOperation::Unlink>(session, buf);
	
	dbg_log (DBG_INFO, "Validate Unlink: %p %s\n", lgc_op->parino_, lgc_op->key_);

	// verify preconditions
	oid = osd::common::ObjectId(lgc_op->parino_);
	if ((ret = lock_verifier_->VerifyLock(session, oid)) < 0) {
		return ret;
	}

	//DirInode* dp = DirInode::Load(session, lgc_op->parino_, &dinode);
	//return dp->Unlink(session, lgc_op->name_);
	// do the operation 
	Table* tp  = Table::Load(session, lgc_op->parino_, &table);
	tp->Unlink(session, lgc_op->key_);

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
