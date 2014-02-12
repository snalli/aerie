#define  __CACHE_GUARD__

#include "osd/containers/name/container.h"
#include <stdint.h>
#include "common/errno.h"
#include "common/prof.h"
#include "osd/main/client/session.h"
#include "osd/main/client/salloc.h"
#include <stdio.h>
//#define PROFILER_SAMPLE __PROFILER_SAMPLE

namespace osd {
namespace containers {
namespace client {


int
NameContainer::VersionManager::return_dentry(void *dentry_list_head_addr)
{
        object()->return_dentry(dentry_list_head_addr);


        struct dentry *it;
        it = ((struct dentry *)dentry_list_head_addr)->next_dentry;
        while(it)
        {
                osd::common::ObjectId *oid = new osd::common::ObjectId(it->val);
                Insert(NULL, it->key, *oid, NULL);
                it->val = (uint64_t) oid;
                it = it->next_dentry;
        }


}

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
                                    osd::common::ObjectId* oidp,
					void  *ip)
{
	int                   ret;
	osd::common::ObjectId tmp_oid;
	ShadowCache::iterator  it;
                        s_log("[%ld] NameContainer::VersionMangager::%s name=%s",s_tid,__func__, name);

	if ((entries_.empty() == false) && ((it = entries_.find(name)) != entries_.end())) {
		if (it->second.present == true) {
			tmp_oid = it->second.oid;
			ip = it->second.ip;
                        s_log("[%ld] NameContainer::VersionMangager::%s(S1) name=%s",s_tid,__func__, name);

		} else {

                        s_log("[%ld] NameContainer::VersionMangager::%s(F1) name=%s",s_tid,__func__, name);
			return -E_NOENT;
		}
	} else {
		if ((ret = object()->Find(session, name, &tmp_oid)) < 0) {
                        s_log("[%ld] NameContainer::VersionMangager::%s(F2) name=%s",s_tid,__func__, name);
			return ret;
		}
                        s_log("[%ld] NameContainer::VersionMangager::%s(S2) name=%s",s_tid,__func__, name);

	}

	*oidp = tmp_oid;

	return E_SUCCESS;
}


// Insert does not allow overwrite. Returns E_EXIST in case the entry 
// already exists
int 
NameContainer::VersionManager::Insert(OsdSession* session, 
                                      const char* name, 
                                      osd::common::ObjectId oid,
					void *ip)
{
//	printf("\nNameContainer::VersionManager::Insert <name : %s, ip : %p>",name, ip);
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
//	if ((ret = object()->Find(session, name, &tmp_oid)) == E_SUCCESS) {
//		return -E_EXIST;
//	}
	
	entry = Shadow(true, oid, ip); 
	psv_entries_count_++;
	std::pair<ShadowCache::iterator, bool> ret_pair = entries_.insert(std::pair<std::string, Shadow>(name, entry));
	assert(ret_pair.second == true);
//printf("\n @ Inside NameContainer::VersionManager::Insert");
	return E_SUCCESS;
}


int 
NameContainer::VersionManager::Erase(OsdSession* session, const char* name)
{
//	printf("\n NameContainer::VersionManager::Erase");
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
	entry = Shadow(false, oid, NULL);
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
