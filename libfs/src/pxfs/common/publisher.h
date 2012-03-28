#ifndef __STAMNOS_PXFS_COMMON_PUBLISHER_H
#define __STAMNOS_PXFS_COMMON_PUBLISHER_H

#include "ssa/main/common/publisher.h"
#include "pxfs/common/types.h"

class Publisher {
public:
	class Messages;
};


class Publisher::Messages {
public:
	typedef ssa::Publisher::Messages::LogicalOperationHeader LogicalOperationHeader;
	struct LogicalOperation;
};

struct Publisher::Messages::LogicalOperation {
	enum OperationCode {
		kLink = 1,
		kWrite
	};
	struct Link;
	struct Write;
};

struct Publisher::Messages::LogicalOperation::Write: public LogicalOperationHeader {
	Write(InodeNumber ino)
		: ino_(ino),
		  LogicalOperationHeader(kWrite, sizeof(Write))
	{ }

	static Write* Load(void* src) {
		return reinterpret_cast<Write*>(src);
	}

	InodeNumber ino_;
};


struct Publisher::Messages::LogicalOperation::Link: public LogicalOperationHeader {
	Link(InodeNumber parino, const char* name, InodeNumber childino)
		: parino_(parino),
		  childino_(childino),
		  LogicalOperationHeader(kLink, sizeof(Link))
	{ 
		strcpy(name_, name); 
	}

	static Link* Load(void* src) {
		return reinterpret_cast<Link*>(src);
	}

	InodeNumber parino_;
	InodeNumber childino_;
	char        name_[32];
};



#endif // __STAMNOS_PXFS_COMMON_PUBLISHER_H
