//! \file
//! Definition of the client side copy-on-write byte container 
//!


#ifndef __STAMNOS_OSD_BYTE_CONTAINER_PROXY_H
#define __STAMNOS_OSD_BYTE_CONTAINER_PROXY_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "bcs/main/common/cdebug.h"
#include "common/util.h"
#include "osd/main/client/session.h"
#include "osd/main/server/session.h"
#include "osd/main/client/rwproxy.h"
#include "osd/containers/byte/common.h"
#include "osd/main/common/obj.h"
#include "osd/main/server/obj.h"
#include "osd/main/server/container.h"


namespace osd {
namespace containers {
namespace client {

typedef ::osd::client::OsdSession OsdSession;

class ByteContainer {
public:
	class VersionManager;

	typedef osd::containers::common::ByteContainer::Object<OsdSession>   Object;
	typedef osd::containers::common::ByteContainer::Region<OsdSession>   Region;
	typedef osd::containers::common::ByteContainer::Slot<OsdSession>     Slot;
	typedef osd::containers::common::ByteContainer::Iterator<OsdSession> Iterator;
	typedef osd::client::rw::ObjectProxy<Object, VersionManager>                 Proxy;
	typedef osd::client::rw::ObjectProxyReference<Object, VersionManager>        Reference;
}; 

// represents the key for the interval [low, high]
class IntervalKey {
public:
	IntervalKey(const int low, const int high)
		: low_(low), 
		  high_(high)
	{ }

	uint64_t Low() const { return low_; };
	uint64_t High() const { return high_; };

protected:
	uint64_t                       low_;
	uint64_t                       high_;
};

struct IntervalKeyCompare {
	// overlapping ranges are considered equivalent
	bool operator()(const IntervalKey& lhv, const IntervalKey& rhv) const
	{
		return lhv.High() < rhv.Low();
	}
};



// represents the payload of the interval [low, high]
class Interval {
public:
	Interval(ByteContainer::Object* obj, const int low, const int high)
		: low_(low), 
		  high_(high),
	          object_(obj)
	{ 
		block_array_ = new char *[high - low + 1];
		for (int i=0; i < high-low+1; i++) {
			block_array_[i] = NULL;
		}
	}

	int Write(OsdSession* session, char*, uint64_t, uint64_t);
	int Read(OsdSession* session, char*, uint64_t, uint64_t);
	int WriteBlock(OsdSession* session, char*, uint64_t, int, int);
	int ReadBlock(OsdSession* session, char*, uint64_t, int, int);

	IntervalKey Key() {
		return IntervalKey(low_, high_);
	}

	void Print() 
	{
		std::cout << "[" << low_ << ", " << high_ << "]" << std::endl;
	}

	uint64_t                       low_;
	uint64_t                       high_;
protected:
	char**                         block_array_;
	ByteContainer::Object*         object_;
};


class IntervalKeyPair: public std::pair<IntervalKey, Interval*> {
public:
	IntervalKeyPair(Interval* interval)
		: std::pair<IntervalKey, Interval*>(interval->Key(), interval)
	{ }
	
	uint64_t Low() const { return first.Low(); }
	uint64_t High() const { return first.High(); }
};

typedef std::map<IntervalKey, Interval*, IntervalKeyCompare> IntervalTree;


class ByteContainer::VersionManager: public osd::vm::client::VersionManager<ByteContainer::Object> {

public:
	 VersionManager()
        {
                printf("\n @ Inside ByteContainer::VersionManager. from src/osd/containers/byte/container-shadow.h");
        }

	int vOpen();
	int vUpdate(OsdSession* session);
	
	int Read(OsdSession* session, char*, uint64_t, uint64_t);
	int Write(OsdSession* session, char*, uint64_t, uint64_t);
	int Size(OsdSession* session);
	int Size();
	void PrintIntervals();

private:
	int ReadShadow(OsdSession* session, char*, uint64_t, uint64_t);
	//int WriteImmutable(OsdSession* session, char*, uint64_t, uint64_t);
	//int WriteMutable(OsdSession* session, char*, uint64_t, uint64_t);

	IntervalTree*      intervaltree_;
	uint64_t           size_;
};


} // namespace client
} // namespace containers
} // namespace osd


namespace osd {
namespace containers {
namespace server {


class ByteContainer {
public:
	typedef osd::containers::common::ByteContainer::Object< ::osd::server::OsdSession> Object;
	
	class Factory: public ::osd::server::ContainerFactory<ByteContainer> {
	};
}; 


} // namespace server
} // namespace containers
} // namespace osd


#endif // __STAMNOS_OSD_BYTE_CONTAINER_PROXY_H
