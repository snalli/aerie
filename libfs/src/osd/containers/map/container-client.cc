#include "osd/containers/map/container.h"
#include <stdint.h>
#include "common/errno.h"
#include "common/prof.h"
#include "osd/main/client/session.h"
#include "osd/main/client/salloc.h"


namespace osd {
namespace containers {
namespace client {


int 
MapContainer::VersionManager::vOpen()
{
	osd::vm::client::VersionManager<MapContainer::Object>::vOpen();
	entries_.clear();
	neg_entries_count_ = 0;

	return 0;
}


int 
MapContainer::VersionManager::vUpdate(OsdSession* session)
{
	osd::vm::client::VersionManager<MapContainer::Object>::vUpdate(session);
	return 0;
}


int
MapContainer::VersionManager::Find(OsdSession* session, 
                                   const char* key, 
                                   osd::common::ObjectId* oidp)
{
	int                   ret;
	osd::common::ObjectId tmp_oid;
	EntryCache::iterator  it;

	// check the private copy first before looking up the global one
	if ((entries_.empty() == false) && ((it = entries_.find(key)) != entries_.end())) {
		if (it->second.first == true) {
			tmp_oid = it->second.second;
		} else {
			return -E_NOENT;
		}
	} else {
		if ((ret = object()->Find(session, key, &tmp_oid)) < 0) {
			return ret;
		}
	}

	*oidp = tmp_oid;

	return E_SUCCESS;
}


// Insert does not allow overwrite. Returns E_EXIST in case the entry 
// already exists
int 
MapContainer::VersionManager::Insert(OsdSession* session, 
                                     const char* key, 
                                     osd::common::ObjectId oid)
{
	EntryCache::iterator   it;
	int                    ret;
	osd::common::ObjectId  tmp_oid;

	if (key[0] == '\0') {
		return -1;
	}

	if ((it = entries_.find(key)) != entries_.end()) {
		if (it->second.first == true) {
			// key exists in the volatile cache
			return -E_EXIST;
		} else {
			// found a negative entry indicating absence due to a previous 
			// erase. just overwrite.
			it->second.first = true;
			it->second.second = oid;
			return E_SUCCESS;
		}
	}

	// check whether key exists in the persistent structure
	if ((ret = object()->Find(session, key, &tmp_oid)) == E_SUCCESS) {
		return -E_EXIST;
	}

	std::pair<EntryCache::iterator, bool> ret_pair = entries_.insert(std::pair<std::string, std::pair<bool, osd::common::ObjectId> >(key, std::pair<bool, osd::common::ObjectId>(true, oid)));
	assert(ret_pair.second == true);
	return E_SUCCESS;
}


int 
MapContainer::VersionManager::Erase(OsdSession* session, const char* key)
{
	EntryCache::iterator   it;
	osd::common::ObjectId  oid;
	int                    ret;

	if (key[0] == '\0') {
		return -1;
	}

	if ((it = entries_.find(key)) != entries_.end()) {
		if (it->second.first == true) {
			// key exists
			entries_.erase(key);
			return E_SUCCESS;
		} else {
			// found a negative entry indicating absence due to a previous unlink
			return -E_NOENT;
		}
	}

	// check whether key exists in the persistent structure
	if ((ret = object()->Find(session, key, &oid)) != E_SUCCESS) {
		return -E_NOENT;
	} 

	// add a negative directory entry indicating absence when removing a 
	// directory entry from the persistent data structure
	std::pair<EntryCache::iterator, bool> ret_pair = entries_.insert(std::pair<std::string, std::pair<bool, osd::common::ObjectId> >(key, std::pair<bool, osd::common::ObjectId>(false, oid)));
	neg_entries_count_++;
	assert(ret_pair.second == true);

	return E_SUCCESS;
}


int
MapContainer::VersionManager::Size(OsdSession* session)
{
	int pos_entries_count_ = entries_.size() - neg_entries_count_;
	return pos_entries_count_ + (object()->Size(session) - neg_entries_count_);
}

} // namespace osd
} // namespace containers
} // namespace client
