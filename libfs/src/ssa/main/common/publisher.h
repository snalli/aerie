#ifndef __STAMNOS_SSA_COMMON_PUBLISHER_H
#define __STAMNOS_SSA_COMMON_PUBLISHER_H

namespace ssa {

class Publisher { 
public:
	class Protocol;
	class Messages;
};


class Publisher::Protocol {
public:
	enum RpcNumbers {
		DEFINE_RPC_NUMBER(SSA_PUBLISHER_PROTOCOL)
	};
};


class Publisher::Messages {
public:
	struct LogicalOpHeader;
	struct CommandHeader;
};


struct Publisher::Messages::LogicalOpHeader {
	LogicalOpHeader(int id)
		: id_(id)
	{ }

	static LogicalOpHeader* Load(void* src) {
		return reinterpret_cast<LogicalOpHeader*>(src);
	}

	union {
		char bytes_[8];
		int  id_; 
	};
};

struct Publisher::Messages::CommandHeader {
	CommandHeader(int id)
		: id_(id)
	{ }

	static CommandHeader* Load(void* src) {
		return reinterpret_cast<CommandHeader*>(src);
	}
	
	union {
		char bytes_[8];
		int  id_; 
	};
};

} // namespace ssa

#endif // __STAMNOS_SSA_COMMON_PUBLISHER_H
