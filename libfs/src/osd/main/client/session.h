#ifndef __STAMNOS_OSD_CLIENT_SESSION_H
#define __STAMNOS_OSD_CLIENT_SESSION_H

#include "osd/main/client/journal.h"
#include "osd/main/client/stm.h"
#include "osd/main/client/stsystem.h"

namespace osd {
namespace client {

class OsdSession {
public:
	OsdSession(osd::client::StorageSystem* stsystem)
		: stsystem_(stsystem),
		  lckmgr_(stsystem->lckmgr()),
		  hlckmgr_(stsystem->hlckmgr()),
		  salloc_(stsystem->salloc()),
		  omgr_(stsystem->omgr())
	{ 
		journal_ = new osd::client::Journal(this);
	}

	osd::client::StorageAllocator* salloc() { return salloc_; }
	osd::client::Journal* journal() { return journal_; }
	osd::client::StorageSystem*      stsystem() { return stsystem_; }
	osd::client::ObjectManager*      omgr() { return stsystem_->omgr(); };

	osd::client::StorageSystem*      stsystem_;
	osd::cc::client::LockManager*    lckmgr_;
	osd::cc::client::HLockManager*   hlckmgr_;
	osd::client::StorageAllocator*   salloc_;
	osd::client::ObjectManager*      omgr_;
	osd::stm::client::Transaction*   tx_;
	osd::client::Journal*            journal_;
};

} // namespace client
} // namespace osd

#endif // __STAMNOS_OSD_CLIENT_SESSION_H
