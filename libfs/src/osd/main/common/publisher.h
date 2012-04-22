#ifndef __STAMNOS_OSD_COMMON_PUBLISHER_H
#define __STAMNOS_OSD_COMMON_PUBLISHER_H

#include "bcs/bcs.h"
#include "osd/main/server/osd-opaque.h"
#include "osd/main/common/obj.h"
#include "osd/main/common/cc.h"

// All structures assume a little endian machine

namespace osd {

class Publisher { 
public:
	class Protocol;
	class Message;
};


class Publisher::Protocol {
public:
	enum RpcNumbers {
		DEFINE_RPC_NUMBER(OSD_PUBLISHER_PROTOCOL)
	};
};


class Publisher::Message {
public:
	enum MessageType {
		kTransactionBegin = 0x1,
		kTransactionCommit,
		kLogicalOperation,
		kContainerOperation,
	};
	struct BaseMessage;
	template<typename T> struct BaseMessageT;
	struct TransactionBegin;
	struct TransactionCommit;
	struct LogicalOperationHeader;
	template<typename T> struct LogicalOperationHeaderT;
	struct LogicalOperation;
	struct ContainerOperationHeader;
	struct ContainerOperation;
};


struct Publisher::Message::BaseMessage {
	BaseMessage(char type = 0)
		: type_(type)
	{ }

	static BaseMessage* Load(void* src) {
		return reinterpret_cast<BaseMessage*>(src);
	}

	char type_;
};


template<typename T>
struct Publisher::Message::BaseMessageT: public BaseMessage {
	BaseMessageT(char type = 0)
		: BaseMessage(type)
	{ }

	static T* Load(void* src) {
		return reinterpret_cast<T*>(src);
	}
};


struct Publisher::Message::TransactionBegin: public BaseMessageT<TransactionBegin> {
	TransactionBegin(int id = 0)
		: BaseMessageT<TransactionBegin>(kTransactionBegin),
		  id_(id)
	{ 
	}

	int id_; 
};


struct Publisher::Message::TransactionCommit: public BaseMessageT<TransactionCommit> {
	TransactionCommit()
		: BaseMessageT<TransactionCommit>(kTransactionCommit)
	{ }
};


struct Publisher::Message::LogicalOperationHeader: public BaseMessageT<LogicalOperationHeader> {
	LogicalOperationHeader(char id, size_t total_size)
		: BaseMessageT<LogicalOperationHeader>(kLogicalOperation),
		  id_(id),
		  payload_size_(total_size - sizeof(LogicalOperationHeader))
	{ }

	char   id_; 
	size_t payload_size_;
};


template<typename T>
struct Publisher::Message::LogicalOperationHeaderT: public LogicalOperationHeader {
	LogicalOperationHeaderT(char id, size_t total_size)
		: LogicalOperationHeader(id, total_size)
	{ }

	static T* Load(void* src) {
		return reinterpret_cast<T*>(src);
	}
};


struct Publisher::Message::ContainerOperationHeader: public BaseMessageT<ContainerOperationHeader> {
	ContainerOperationHeader(char id, short total_size)
		: BaseMessageT<ContainerOperationHeader>(kContainerOperation),
		  id_(id),
		  payload_size_(total_size - sizeof(ContainerOperationHeader))
	{ }
	
	char  id_; 
	short payload_size_;
};


struct Publisher::Message::LogicalOperation {
	enum OperationCode {
		kAllocContainer = 1, 
	};
	struct AllocContainer;
};


struct Publisher::Message::LogicalOperation::AllocContainer: public osd::Publisher::Message::LogicalOperationHeaderT<AllocContainer> {
	AllocContainer(int capability, osd::common::ObjectId oid, int index_hint)
		: osd::Publisher::Message::LogicalOperationHeaderT<AllocContainer>(kAllocContainer, sizeof(AllocContainer)),
		  capability_(capability),
		  oid_(oid),
		  index_hint_(index_hint)
	{ }

	int                   capability_;
	int                   index_hint_;
	osd::common::ObjectId oid_;
};


struct Publisher::Message::ContainerOperation {
	enum OperationCode {
		kAllocateExtent = 1,
		kLinkBlock,
		kLockCertificate
	};
	struct AllocateExtent;
	struct LinkBlock;
	struct LockCertificate;
};


struct Publisher::Message::ContainerOperation::AllocateExtent: public ContainerOperationHeader {
	AllocateExtent(int capability, osd::common::ExtentId eid, int index_hint)
		: ContainerOperationHeader(kAllocateExtent, sizeof(AllocateExtent)),
		  capability_(capability),
		  eid_(eid),
		  index_hint_(index_hint)
	{ }

	static AllocateExtent* Load(void* src) {
		return reinterpret_cast<AllocateExtent*>(src);
	}

	int                   capability_;
	int                   index_hint_;
	osd::common::ExtentId eid_;
};


struct Publisher::Message::ContainerOperation::LinkBlock: public ContainerOperationHeader {
	LinkBlock(osd::common::ObjectId oid, uint64_t bn, void* ptr)
		: ContainerOperationHeader(kLinkBlock, sizeof(LinkBlock)),
		  oid_(oid),
		  bn_(bn), 
		  ptr_(ptr)
	{ }
	
	static LinkBlock* Load(void* src) {
		return reinterpret_cast<LinkBlock*>(src);
	}
	
	osd::common::ObjectId oid_;
	uint64_t              bn_;
	void*                 ptr_;
};


struct Publisher::Message::ContainerOperation::LockCertificate: public ContainerOperationHeader {
	LockCertificate(int nlocks)
		: ContainerOperationHeader(kLockCertificate, sizeof(LockCertificate) + nlocks*sizeof(osd::cc::common::LockId))
	{ }
	
	static LockCertificate* Load(void* src) {
		return reinterpret_cast<LockCertificate*>(src);
	}
	
	osd::cc::common::LockId locks_[0];
};


} // namespace osd

#endif // __STAMNOS_OSD_COMMON_PUBLISHER_H
