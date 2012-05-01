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



// FIXME: Currently we publish by simply doing the updates in-place. 
// Normally this must be done via the trusted server using the journal 
int 
NameContainer::VersionManager::vUpdate(OsdSession* session)
{
	EntryCache::iterator  it;
	int                   ret;

	osd::vm::client::VersionManager<NameContainer::Object>::vUpdate(session);

	for (it = entries_.begin(); it != entries_.end(); it++) {
		if (it->second.first == true) {
			// normal entry -- link
			// FIXME: if a link followed an unlink then the entry is 
			// marked as valid (true). Adding this entry to the persistent 
			// directory without removing the previous one is incorrect.
			// Currently we don't have a way to detect this case but
			// this case won't be a problem  when doing the publish through the server
			// because there we replay the journal which has the unlink operation.
			// so don't worry for now.
			// TEST TestLinkPublish3 checks this case
			printf("Publish: %s -> %lx\n", it->first.c_str(), it->second.second.u64());
			if ((ret = object()->Insert(session, it->first.c_str(), it->second.second)) != 0) {
				return ret;
			}
		} else {
			printf("REMOVE\n");
			// negative entry -- remove
			if ((ret = object()->Erase(session, it->first.c_str())) != 0) {
				return ret;
			}
		}
	}
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
		if (it->second.first == true) {
			tmp_oid = it->second.second;
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
	PROFILER_PREAMBLE
	EntryCache::iterator   it;
	int                    ret;
	osd::common::ObjectId  tmp_oid;

	if (name[0] == '\0') {
		return -1;
	}

	PROFILER_SAMPLE
	if ((it = entries_.find(name)) != entries_.end()) {
		if (it->second.first == true) {
			// name exists in the volatile cache
			return -E_EXIST;
		} else {
			// found a negative entry indicating absence due to a previous 
			// unlink. just overwrite.
			printf(">>>>>>>>>\n");
			printf(">>>>>>>>> Insert: %p\n", oid.u64());
			printf(">>>>>>>>>\n");
			it->second.first = true;
			it->second.second = oid;
			return E_SUCCESS;
		}
	}

	PROFILER_SAMPLE
	// check whether name exists in the persistent structure
	if ((ret = object()->Find(session, name, &tmp_oid)) == E_SUCCESS) {
		return -E_EXIST;
	}
	PROFILER_SAMPLE

	std::pair<EntryCache::iterator, bool> ret_pair = entries_.insert(std::pair<std::string, std::pair<bool, osd::common::ObjectId> >(name, std::pair<bool, osd::common::ObjectId>(true, oid)));
	assert(ret_pair.second == true);
	PROFILER_SAMPLE
	return E_SUCCESS;
}


int 
NameContainer::VersionManager::Erase(OsdSession* session, const char* name)
{
	EntryCache::iterator   it;
	osd::common::ObjectId  oid;
	int                    ret;

	if (name[0] == '\0') {
		return -1;
	}

	if ((it = entries_.find(name)) != entries_.end()) {
		if (it->second.first == true) {
			// name exists
		
			printf(">>>>>>>>>\n");
			printf(">>>>>>>>> Erase: %p\n", it->second.second.u64());
			printf(">>>>>>>>>\n");
			entries_.erase(name);
			return E_SUCCESS;
		} else {
			// found a negative entry indicating absence due to a previous unlink
			return -E_NOENT;
		}
	}

	// check whether name exists in the persistent structure
	if ((ret = object()->Find(session, name, &oid)) != E_SUCCESS) {
		return -E_NOENT;
	} 

	// add a negative directory entry indicating absence when removing a 
	// directory entry from the persistent data structure
	std::pair<EntryCache::iterator, bool> ret_pair = entries_.insert(std::pair<std::string, std::pair<bool, osd::common::ObjectId> >(name, std::pair<bool, osd::common::ObjectId>(false, oid)));
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
