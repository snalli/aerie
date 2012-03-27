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
		kWrite = 1
	};
	struct Write;
};

struct Publisher::Messages::LogicalOperation::Write: public LogicalOperationHeader {
	Write(InodeNumber)
		: i_(0),
		  LogicalOperationHeader(kWrite)
	{ }

	static Write* Load(void* src) {
		return reinterpret_cast<Write*>(src);
	}

	int i_;
};


#endif // __STAMNOS_PXFS_COMMON_PUBLISHER_H
