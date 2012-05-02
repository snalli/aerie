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
	neg_entries_count_ = 0;

	return 0;
}


int 
NameContainer::VersionManager::vUpdate(OsdSession* session)
{
	EntryCache::iterator  it;
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
	EntryCache::iterator  it;

	// check the private copy first before looking up the global one
	if ((entries_.empty() == false) && ((it = entries_.find(name)) != entries_.end())) {
		if (it->second.created == true) {
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
	EntryCache::iterator   it;
	int                    ret;
	osd::common::ObjectId  tmp_oid;
	Entry                  entry;

	if (name[0] == '\0') {
		return -1;
	}

	// check lookaside buffer 
	if ((it = entries_.find(name)) != entries_.end()) {
		if (it->second.created == false) {
			psv_entries_count_++;
		}
		it->second.created = true;
		it->second.oid = oid;
		return E_SUCCESS;
	}

	// no entry in the lookaside buffer 
	// check whether name exists in the persistent structure
	if ((ret = object()->Find(session, name, &tmp_oid)) == E_SUCCESS) {
		// we overwrite an existing name so we mark it both as deleted and created
		entry = Entry(true, true, oid); 
		psv_entries_count_++;
		neg_entries_count_++;
	} else {
		entry = Entry(false, true, oid);
		psv_entries_count_++;
	}

	std::pair<EntryCache::iterator, bool> ret_pair = entries_.insert(std::pair<std::string, Entry>(name, entry));
	assert(ret_pair.second == true);
	return E_SUCCESS;
}


int 
NameContainer::VersionManager::Erase(OsdSession* session, const char* name)
{
	EntryCache::iterator   it;
	osd::common::ObjectId  oid;
	int                    ret;
	Entry                  entry;

	if (name[0] == '\0') {
		return -1;
	}

	// check lookaside buffer 
	if ((it = entries_.find(name)) != entries_.end()) {
		if (it->second.deleted == true) {
			// found a negative entry indicating absence due to a previous unlink
			if (it->second.created == true) {
				it->second.created = false;
				psv_entries_count_--;
				return E_SUCCESS;
			} else {
				return -E_NOENT;
			}
		} else {
			psv_entries_count_--;
			entries_.erase(name);
			return E_SUCCESS;
		}
	}

	// no entry in the lookaside buffer 
	// check whether name exists in the persistent structure
	if ((ret = object()->Find(session, name, &oid)) != E_SUCCESS) {
		return -E_NOENT;
	} 

	// add a negative directory entry indicating absence when removing a 
	// directory entry from the persistent data structure
	entry = Entry(true, false, oid);
	std::pair<EntryCache::iterator, bool> ret_pair = entries_.insert(std::pair<std::string, Entry>(name, entry));
	neg_entries_count_++;
	assert(ret_pair.second == true);

	return E_SUCCESS;
}


int
NameContainer::VersionManager::Size(OsdSession* session)
{
	int pos_entries_count_ = entries_.size() - neg_entries_count_;
	return pos_entries_count_ + (object()->Size(session) - neg_entries_count_);
}

} // namespace osd
} // namespace containers
} // namespace client
