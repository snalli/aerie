#ifndef __STAMNOS_KVFS_COMMON_PUBLISHER_H
#define __STAMNOS_KVFS_COMMON_PUBLISHER_H

#include "osd/main/common/publisher.h"
#include "kvfs/common/types.h"

class Publisher {
public:
	class Message;
};


class Publisher::Message {
public:
	typedef osd::Publisher::Message::LogicalOperationHeader LogicalOperationHeader;
	struct LogicalOperation;
};


struct Publisher::Message::LogicalOperation {
	enum OperationCode {
		kMakeFile = 10,
		kUnlink
	};
	struct MakeFile;
	struct Unlink;
};


struct Publisher::Message::LogicalOperation::MakeFile: public osd::Publisher::Message::LogicalOperationHeaderT<MakeFile> {
	MakeFile(InodeNumber parino, const char* key, InodeNumber childino)
		: osd::Publisher::Message::LogicalOperationHeaderT<MakeFile>(kMakeFile, sizeof(MakeFile)),
		  parino_(parino),
		  childino_(childino)
	{ 
		strcpy(key_, key); 
	}

	InodeNumber parino_;
	InodeNumber childino_;
	char        key_[32];
};


struct Publisher::Message::LogicalOperation::Unlink: public osd::Publisher::Message::LogicalOperationHeaderT<Unlink> {
	Unlink(InodeNumber parino, const char* key)
		: osd::Publisher::Message::LogicalOperationHeaderT<Unlink>(kUnlink, sizeof(Unlink)),
		  parino_(parino)
	{ 
		strcpy(key_, key); 
	}

	InodeNumber parino_;
	char        key_[32];
};



#endif // __STAMNOS_KVFS_COMMON_PUBLISHER_H
