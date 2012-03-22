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
	struct LogicalOp;
};


struct Publisher::Messages::LogicalOp {


};


} // namespace ssa

#endif // __STAMNOS_SSA_COMMON_PUBLISHER_H
