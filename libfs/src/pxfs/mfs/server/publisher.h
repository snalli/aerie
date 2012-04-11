#ifndef __STAMNOS_PXFS_MFS_SERVER_PUBLISHER_H
#define __STAMNOS_PXFS_MFS_SERVER_PUBLISHER_H

#include "osd/main/server/osd-opaque.h"
#include "pxfs/common/publisher.h"


namespace server {

class WriteVerifier;
class LockVerifier;

class Publisher {
public:
	static int Init();
	static int Register(::osd::server::StorageSystem* stsystem);
	static int MakeFile(::osd::server::OsdSession* session, char* lgc_op_hdr, ::osd::Publisher::Message::BaseMessage* next);
	static int MakeDir(::osd::server::OsdSession* session, char* lgc_op_hdr, ::osd::Publisher::Message::BaseMessage* next);
	static int Link(::osd::server::OsdSession* session, char* lgc_op_hdr, ::osd::Publisher::Message::BaseMessage* next);
	static int Unlink(::osd::server::OsdSession* session, char* lgc_op_hdr, ::osd::Publisher::Message::BaseMessage* next);
	static int Write(::osd::server::OsdSession* session, char* lgc_op_hdr, ::osd::Publisher::Message::BaseMessage* next);

private:
	static WriteVerifier* write_verifier_;
	static LockVerifier*  lock_verifier_;
};


} // namespace server

#endif  // __STAMNOS_PXFS_MFS_SERVER_PUBLISHER_H
