#ifndef __STAMNOS_OSD_SERVER_SESSION_H
#define __STAMNOS_OSD_SERVER_SESSION_H

#include "osd/main/common/obj.h"
#include "bcs/main/server/session.h"
#include "osd/main/server/osd-opaque.h"
#include "osd/main/server/shbuf.h"

namespace osd {
namespace server {

/**< Storage System Abstractions layer session */
class OsdSession: public ::server::BcsSession {
public:

	osd::server::StorageAllocator* salloc();

	int Init(int clt);

//protected:
	osd::server::OsdSharedBuffer*      shbuf_;
	osd::server::StorageSystem*        storage_system_;
	std::vector<osd::common::ObjectId> sets_; // sets of pre-allocated containers to client
};


} // namespace osd
} // namespace server

#endif // __STAMNOS_OSD_SERVER_SESSION_H
