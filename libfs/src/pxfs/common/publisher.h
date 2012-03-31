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
		kMakeFile = 1,
		kMakeDir,
		kLink,
		kUnlink,
		kWrite
	};
	struct MakeFile;
	struct MakeDir;
	struct Link;
	struct Unlink;
	struct Write;
};


struct Publisher::Messages::LogicalOperation::Write: public ssa::Publisher::Messages::LogicalOperationHeaderT<Write> {
	Write(InodeNumber ino)
		: ino_(ino),
		  ssa::Publisher::Messages::LogicalOperationHeaderT<Write>(kWrite, sizeof(Write))
	{ }

	InodeNumber ino_;
};


struct Publisher::Messages::LogicalOperation::MakeFile: public ssa::Publisher::Messages::LogicalOperationHeaderT<MakeFile> {
	MakeFile(InodeNumber parino, const char* name, InodeNumber childino)
		: parino_(parino),
		  childino_(childino),
		  ssa::Publisher::Messages::LogicalOperationHeaderT<MakeFile>(kMakeFile, sizeof(MakeFile))
	{ 
		strcpy(name_, name); 
	}

	InodeNumber parino_;
	InodeNumber childino_;
	char        name_[32];
};


struct Publisher::Messages::LogicalOperation::MakeDir: public ssa::Publisher::Messages::LogicalOperationHeaderT<MakeDir> {
	MakeDir(InodeNumber parino, const char* name, InodeNumber childino)
		: parino_(parino),
		  childino_(childino),
		  ssa::Publisher::Messages::LogicalOperationHeaderT<MakeDir>(kMakeDir, sizeof(MakeDir))
	{ 
		strcpy(name_, name); 
	}

	InodeNumber parino_;
	InodeNumber childino_;
	char        name_[32];
};


struct Publisher::Messages::LogicalOperation::Link: public ssa::Publisher::Messages::LogicalOperationHeaderT<Link> {
	Link(InodeNumber parino, const char* name, InodeNumber childino)
		: parino_(parino),
		  childino_(childino),
		  ssa::Publisher::Messages::LogicalOperationHeaderT<Link>(kLink, sizeof(Link))
	{ 
		strcpy(name_, name); 
	}

	InodeNumber parino_;
	InodeNumber childino_;
	char        name_[32];
};


struct Publisher::Messages::LogicalOperation::Unlink: public ssa::Publisher::Messages::LogicalOperationHeaderT<Unlink> {
	Unlink(InodeNumber parino, const char* name)
		: parino_(parino),
		  ssa::Publisher::Messages::LogicalOperationHeaderT<Unlink>(kUnlink, sizeof(Unlink))
	{ 
		strcpy(name_, name); 
	}

	InodeNumber parino_;
	char        name_[32];
};


#endif // __STAMNOS_PXFS_COMMON_PUBLISHER_H
