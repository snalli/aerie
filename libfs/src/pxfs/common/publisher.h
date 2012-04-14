#ifndef __STAMNOS_PXFS_COMMON_PUBLISHER_H
#define __STAMNOS_PXFS_COMMON_PUBLISHER_H

#include "osd/main/common/publisher.h"
#include "pxfs/common/types.h"

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


struct Publisher::Message::LogicalOperation::Write: public osd::Publisher::Message::LogicalOperationHeaderT<Write> {
	Write(InodeNumber ino)
		: osd::Publisher::Message::LogicalOperationHeaderT<Write>(kWrite, sizeof(Write)),
		  ino_(ino)
	{ }

	InodeNumber ino_;
};


struct Publisher::Message::LogicalOperation::MakeFile: public osd::Publisher::Message::LogicalOperationHeaderT<MakeFile> {
	MakeFile(InodeNumber parino, const char* name, InodeNumber childino)
		: osd::Publisher::Message::LogicalOperationHeaderT<MakeFile>(kMakeFile, sizeof(MakeFile)),
		  parino_(parino),
		  childino_(childino)
	{ 
		strcpy(name_, name); 
	}

	InodeNumber parino_;
	InodeNumber childino_;
	char        name_[32];
};


struct Publisher::Message::LogicalOperation::MakeDir: public osd::Publisher::Message::LogicalOperationHeaderT<MakeDir> {
	MakeDir(InodeNumber parino, const char* name, InodeNumber childino)
		: osd::Publisher::Message::LogicalOperationHeaderT<MakeDir>(kMakeDir, sizeof(MakeDir)),
		  parino_(parino),
		  childino_(childino)
	{ 
		strcpy(name_, name); 
	}

	InodeNumber parino_;
	InodeNumber childino_;
	char        name_[32];
};


struct Publisher::Message::LogicalOperation::Link: public osd::Publisher::Message::LogicalOperationHeaderT<Link> {
	Link(InodeNumber parino, const char* name, InodeNumber childino)
		: osd::Publisher::Message::LogicalOperationHeaderT<Link>(kLink, sizeof(Link)),
		  parino_(parino),
		  childino_(childino)
	{ 
		strcpy(name_, name); 
	}

	InodeNumber parino_;
	InodeNumber childino_;
	char        name_[32];
};


struct Publisher::Message::LogicalOperation::Unlink: public osd::Publisher::Message::LogicalOperationHeaderT<Unlink> {
	Unlink(InodeNumber parino, const char* name)
		: osd::Publisher::Message::LogicalOperationHeaderT<Unlink>(kUnlink, sizeof(Unlink)),
		  parino_(parino)
	{ 
		strcpy(name_, name); 
	}

	InodeNumber parino_;
	char        name_[32];
};


#endif // __STAMNOS_PXFS_COMMON_PUBLISHER_H
