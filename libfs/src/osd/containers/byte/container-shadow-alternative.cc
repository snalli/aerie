#include "osd/containers/byte/container.h"
#include <stdint.h>
#include <inttypes.h>
#include <vector>
#include "common/errno.h"
#include "osd/main/client/session.h"
#include "osd/main/client/salloc.h"
#include "osd/main/common/const.h"
#include "osd/main/common/publisher.h"
#include "common/interval_tree.h"
#include "scm/const.h"

namespace osd {
namespace containers {
namespace client {

static uint64_t __counter = 0;


#define min(a,b) ((a) < (b)? (a) : (b))
#define max(a,b) ((a) > (b)? (a) : (b))

// It's possible that we create an interval that is larger than we need. 
// In such case we only allocate storage to the blocks we actually write.
// However we should be careful when sharing to release all the shadow
// intervals. The problem is that someone else may allocate storage
// to blocks covered by an existing shadow interval. In such a case
// we won't see the new blocks because our shadow does not have them.
// This problem is solved by invalidating the container when releasing
// the lock.


// block is absolute number (relative to the container)
// offset is relative to block bn
int
Interval::WriteBlock(OsdSession* session, char* src, uint64_t bn, int off, int n)
{
	int   ret;
	char* bp;
	void* ptr = NULL;

	if (low_ > bn || bn > high_) {
		return -1;
	}


	if (!(bp = block_array_[bn - low_])) {
		if ((ret = session->salloc()->AllocateExtent(session, kBlockSize, 
		                                             kData, &ptr)) < 0)
		{ 
			printf("OUT OF SPACE\n");
			return ret;
		}
		// The allocator journaled the allocation. We just journal the data block link.
		session->journal() << osd::Publisher::Message::ContainerOperation::LinkBlock(object_->oid(), bn, ptr);
		bp = block_array_[bn - low_] = (char*) ptr;
		// Zero the part of the newly allocated block that is not written to
		// ensure we later read zeros and not garbage.
		if (off>0) {
			memset(bp, 0, off);
		}	
		memset(&bp[off+n], 0, kBlockSize-n); 
	}

	//memmove(&bp[off], src, n);
	scm_memcpy(&bp[off], src, n);
	return n;
}


int
Interval::Write(OsdSession* session, char* src, uint64_t off, uint64_t n)
{
	return osd::containers::common::__Write<OsdSession, Interval> (session, this, src, off, n);
}


int
Interval::ReadBlock(OsdSession* session, char* dst, uint64_t bn, int off, int n)
{
	char* bp;

	if (low_ > bn || bn > high_) {
		return -1;
	}

	if (!(bp = block_array_[bn - low_])) {
		memset(dst, 0, n);
		return n;
	}

	memmove(dst, &bp[off], n);
	return n;
}


int
Interval::Read(OsdSession* session, char* dst, uint64_t off, uint64_t n)
{
	return osd::containers::common::__Read<OsdSession, Interval> (session, this, dst, off, n);
}


/////////////////////////////////////////////////////////////////////////////
// 
// ByteContainer Verion Manager: Logical Copy-On-Write 
//
/////////////////////////////////////////////////////////////////////////////


int 
ByteContainer::VersionManager::vOpen()
{
	osd::vm::client::VersionManager<ByteContainer::Object>::vOpen();
	
	intervaltree_ = new IntervalTree();
	size_ = object()->Size();

	return E_SUCCESS;
}


int 
ByteContainer::VersionManager::vUpdate(OsdSession* session)
{
	osd::vm::client::VersionManager<ByteContainer::Object>::vUpdate(session);

	// do nothing
	
	return 0;
}


// it first tries to read from the shadow buffer before reading 
// from the immutable image
int 
ByteContainer::VersionManager::ReadShadow(OsdSession* session, char* dst, 
                                          uint64_t off, uint64_t n)
{
	int                     ret = 0;
	int                     w;
	Interval*               interval;
	uint64_t                tot;
	uint64_t                m;
	uint64_t                bn;
	uint64_t                high_bn;
	uint64_t                persistent_lb_bn; // lower bound of the persistent container
	uint64_t                shadow_lb_bn;     // lower bound of the shadow container
	uint64_t                upper_bn;
	bool                    return_zeros;
	IntervalTree::iterator  interval_it;
	IntervalTree::iterator  lb_interval_it; // lower bound interval

	dbg_log (DBG_DEBUG, "Read range = [%" PRIu64 ", %" PRIu64 "] n=%" PRIu64 " (size=%" PRIu64 "\n", off, off+n-1, n, size_);
	
	high_bn = (off + n - 1) / kBlockSize;
	for (tot=0; tot < n; tot+=m, off+=m) 
	{
		bn = off/kBlockSize;
		// if bn falls into an interval then lower_bound returns that interval
		// because bn is less than the high bound of that interval
		lb_interval_it = intervaltree_->lower_bound(IntervalKey(bn, bn));
		if (lb_interval_it != intervaltree_->end()) {
			shadow_lb_bn = (*lb_interval_it).first.Low();
		} else {
			shadow_lb_bn = -1; /* overflow it to make comparison checks easier */
		}
		if (object()->LowerBound(session, bn, &persistent_lb_bn) != 0) {
			persistent_lb_bn = -1; 
		}
		if (shadow_lb_bn == -1 && persistent_lb_bn == -1) {
			if (off >= size_) {
				break;
			}
			ret = size_ - off;
			memset(&dst[tot], 0, ret);
		} else if (bn >= shadow_lb_bn && bn <= (*lb_interval_it).first.High() && 0) {
			ret = (*lb_interval_it).second->Read(session, &dst[tot], off, n-tot);
		} else if (bn == persistent_lb_bn) {
			ret = object()->Read(session, &dst[tot], off, n - tot);
		} else { 
			if (shadow_lb_bn > persistent_lb_bn) {
				upper_bn = persistent_lb_bn - 1;
			} else {
				upper_bn = shadow_lb_bn - 1;
			}
			ret = min((upper_bn + 1) * kBlockSize - off, n - tot);
			memset(&dst[tot], 0, ret);
		}
		if (ret <= 0) {
			break;
		}
		m = ret;
	}
	return tot;
}


int 
ByteContainer::VersionManager::Read(OsdSession* session, char* dst, 
                                    uint64_t off, uint64_t n)
{
	if (intervaltree_->empty()) {
		return object()->Read(session, dst, off, n);
	}
	return ReadShadow(session, dst, off, n);
}


static bool __debug = false;

void trigger_debug(osd::containers::client::ByteContainer::Object* object)
{
	if (osd::containers::client::ByteContainer::Object* object = (osd::containers::client::ByteContainer::Object*) 0x80fffc0700) {
		if (object->Size() > 4096*8) {
			__debug = true;
		}
	}
}

void print_debug(OsdSession* session, osd::containers::client::ByteContainer::Object* object, bool incr_counter=true) {
	if (incr_counter) __counter++;
	if (__debug) {
		printf("[%llu] Object: %p\n", __counter, object);
		osd::containers::client::ByteContainer::Object* tmp_object = (osd::containers::client::ByteContainer::Object*) 0x80fffc0700;
		tmp_object->PrintBlocks(session);
	}
}


void print_culprit() {
	if (__debug) {
		osd::containers::client::ByteContainer::Object* tmp_object = (osd::containers::client::ByteContainer::Object*) 0x80fffc0700;
		tmp_object->PrintBlocks(NULL);
	}
}

/**
 * off offset is relative to 0 (beginning of container)
 */
int 
ByteContainer::VersionManager::Write(OsdSession* session, 
                                     char* src, 
                                     uint64_t off, 
                                     uint64_t n)
{
	int                     ret = 0;
	int                     w;
	Interval*               interval;
	uint64_t                tot;
	uint64_t                m;
	uint64_t                bn;
	uint64_t                high_bn;
	uint64_t                persistent_lb_bn; // lower bound of the persistent container
	uint64_t                shadow_lb_bn;     // lower bound of the shadow container
	uint64_t                upper_bn;
	IntervalTree::iterator  interval_it;
	IntervalTree::iterator  lb_interval_it; // lower bound interval

	dbg_log (DBG_DEBUG, "Write range = [%" PRIu64 " , %" PRIu64 " ] n=%" PRIu64 "\n", off, off+n-1, n);
	//printf ("Write object=%p, range = [%" PRIu64 " , %" PRIu64 " ] n=%" PRIu64 "\n", object(), off, off+n-1, n);
	
	high_bn = (off + n - 1) / kBlockSize;

	//print_debug(session, object());
	//trigger_debug(object());
	
	for (tot=0; tot < n; tot+=m, off+=m) 
	{
		bn = off/kBlockSize;
		lb_interval_it = intervaltree_->lower_bound(IntervalKey(bn, bn));
		if (lb_interval_it != intervaltree_->end()) {
			shadow_lb_bn = (*lb_interval_it).first.Low();
		} else {
			shadow_lb_bn = -1; /* overflow it to make comparison checks easier */
		}
		if (object()->LowerBound(session, bn, &persistent_lb_bn) != 0) {
			persistent_lb_bn = -1; /* overflow it */
		}
		if (shadow_lb_bn == -1 && persistent_lb_bn == -1) {
			upper_bn = high_bn;
			interval = new Interval(object(), bn, upper_bn);
			intervaltree_->insert(IntervalKeyPair(interval));
			ret = interval->Write(session, &src[tot], off, n-tot);
		} else if (bn >= shadow_lb_bn && bn <= (*lb_interval_it).first.High()) {
			ret = (*lb_interval_it).second->Write(session, &src[tot], off, n-tot);
		} else if (bn == persistent_lb_bn) {
			ret = object()->Write(session, &src[tot], off, n - tot);
		} else { 
			if (shadow_lb_bn > persistent_lb_bn) {
				upper_bn = persistent_lb_bn - 1;
			} else {
				upper_bn = shadow_lb_bn - 1;
			}
			assert (upper_bn >= bn);
			interval = new Interval(object(), bn, upper_bn);
			intervaltree_->insert(IntervalKeyPair(interval));
			ret = interval->Write(session, &src[tot], off, n-tot);
		}
		if (ret < 0) {
			break;
		}
		m = ret;
	}
	//print_debug(session, object());

	if (ret < 0 && tot == 0) {
		return -1;
	}

	if ( (off) > size_ ) {
		size_ = off;
	}

	return tot;
}


int
ByteContainer::VersionManager::Size(OsdSession* session)
{
	return size_;
}


int
ByteContainer::VersionManager::Size()
{
	return size_;
}

void
ByteContainer::VersionManager::PrintIntervals()
{
	IntervalTree::iterator  interval_it;

	for (interval_it = intervaltree_->begin(); interval_it != intervaltree_->end(); 
	     interval_it++) 
	{
		(*interval_it).second->Print();
	}
}


} // namespace osd
} // namespace containers
} // namespace client
