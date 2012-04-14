#ifndef __STAMNOS_KVFS_SERVER_PUBLISHER_H
#define __STAMNOS_KVFS_SERVER_PUBLISHER_H

#include "osd/main/server/osd-opaque.h"
#include "kvfs/common/publisher.h"


namespace server {

class WriteVerifier;
class LockVerifier;

class Publisher {
public:
	static int Init();
	static int Register(::osd::server::StorageSystem* stsystem);
	static int MakeFile(::osd::server::OsdSession* session, char* lgc_op_hdr, ::osd::Publisher::Message::BaseMessage* next);
	static int Unlink(::osd::server::OsdSession* session, char* lgc_op_hdr, ::osd::Publisher::Message::BaseMessage* next);

private:
	static WriteVerifier* write_verifier_;
	static LockVerifier*  lock_verifier_;
};


} // namespace server

#endif  // __STAMNOS_KVFS_SERVER_PUBLISHER_H
