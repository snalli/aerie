#include "osd/containers/name/container.h"
#include <stdint.h>
#include "common/errno.h"
#include "common/prof.h"
#include "osd/main/client/session.h"
#include "osd/main/client/salloc.h"

//#define PROFILER_SAMPLE __PROFILER_SAMPLE

namespace osd {
namespace containers {
namespace client {


int 
NameContainer::VersionManager::vOpen()
{
	osd::vm::client::VersionManager<NameContainer::Object>::vOpen();
	entries_.clear();
	ngv_entries_count_ = 0;
	psv_entries_count_ = 0;

	return 0;
}


int 
NameContainer::VersionManager::vUpdate(OsdSession* session)
{
	ShadowCache::iterator  it;
	int                   ret;

	osd::vm::client::VersionManager<NameContainer::Object>::vUpdate(session);

	return 0;
}


int
NameContainer::VersionManager::Find(OsdSession* session, 
                                    const char* name, 
                                    osd::common::ObjectId* oidp)
{
	int                   ret;
	osd::common::ObjectId tmp_oid;
	ShadowCache::iterator  it;

	// check the private copy first before looking up the global one
	if ((entries_.empty() == false) && ((it = entries_.find(name)) != entries_.end())) {
		if (it->second.present == true) {
			tmp_oid = it->second.oid;
		} else {
			return -E_NOENT;
		}
	} else {
		if ((ret = object()->Find(session, name, &tmp_oid)) < 0) {
			return ret;
		}
	}

	*oidp = tmp_oid;

	return E_SUCCESS;
}


// Insert does not allow overwrite. Returns E_EXIST in case the entry 
// already exists
int 
NameContainer::VersionManager::Insert(OsdSession* session, 
                                      const char* name, 
                                      osd::common::ObjectId oid)
{
	ShadowCache::iterator  it;
	int                    ret;
	osd::common::ObjectId  tmp_oid;
	Shadow                 entry;

	if (name[0] == '\0') {
		return -1;
	}

	// check lookaside buffer 
	if ((it = entries_.find(name)) != entries_.end()) {
		if (it->second.present == true) {
			return -E_EXIST;
		} else {
			it->second.present = true;
			it->second.oid = oid;
			psv_entries_count_++;
			ngv_entries_count_--;
			return E_SUCCESS;
		}
	}

	// no entry in the lookaside buffer 
	// check whether name exists in the persistent structure
	if ((ret = object()->Find(session, name, &tmp_oid)) == E_SUCCESS) {
		return -E_EXIST;
	}
	
	entry = Shadow(true, oid); 
	psv_entries_count_++;
	std::pair<ShadowCache::iterator, bool> ret_pair = entries_.insert(std::pair<std::string, Shadow>(name, entry));
	assert(ret_pair.second == true);
	return E_SUCCESS;
}


int 
NameContainer::VersionManager::Erase(OsdSession* session, const char* name)
{
	ShadowCache::iterator  it;
	osd::common::ObjectId  oid;
	int                    ret;
	Shadow                 entry;

	if (name[0] == '\0') {
		return -1;
	}

	// check lookaside buffer 
	if ((it = entries_.find(name)) != entries_.end()) {
		if (it->second.present == true) {
			it->second.present = false;
			psv_entries_count_--;
			ngv_entries_count_++;
			return E_SUCCESS;
		} else {
			return -E_NOENT;
		}
	}

	// no entry in the lookaside buffer 
	// check whether name exists in the persistent structure
	if ((ret = object()->Find(session, name, &oid)) != E_SUCCESS) {
		return -E_NOENT;
	} 

	// add a negative directory entry indicating absence when removing a 
	// directory entry from the persistent data structure
	entry = Shadow(false, oid);
	std::pair<ShadowCache::iterator, bool> ret_pair = entries_.insert(std::pair<std::string, Shadow>(name, entry));
	ngv_entries_count_++;
	assert(ret_pair.second == true);

	return E_SUCCESS;
}


int
NameContainer::VersionManager::Size(OsdSession* session)
{
	return psv_entries_count_ + (object()->Size(session) - ngv_entries_count_);
}

} // namespace osd
} // namespace containers
} // namespace client
