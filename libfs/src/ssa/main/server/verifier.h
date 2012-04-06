/**
 * \brief Parses and verifies a stream of container_operations
 */

#ifndef __STAMNOS_SSA_SERVER_CONTAINER_VERIFIER_H
#define __STAMNOS_SSA_SERVER_CONTAINER_VERIFIER_H

#include "ssa/main/common/obj.h"
#include "ssa/main/server/session.h"
#include "ssa/main/common/publisher.h"

typedef int (*ActionFunction)(ssa::server::SsaSession* session, char* buf, ssa::Publisher::Messages::ContainerOperationHeader*);

template<typename T, typename A>
int Action(ssa::server::SsaSession* session, 
           char* buf, 
           ssa::Publisher::Messages::ContainerOperationHeader* header) 
{
	ssa::server::SsaSharedBuffer* shbuf = session->shbuf_;
	size_t                        size = header->payload_size_;
	T*                            physical_op_msg;
	if (size > 0) {
		shbuf->Read(&buf[sizeof(*header)], size);
		physical_op_msg = T::Load(buf);
	}
	return A::Action(session, physical_op_msg);
};


namespace server {


class Verifier {
public:
	int Parse(::ssa::server::SsaSession* session, ::ssa::Publisher::Messages::BaseMessage* next);
	
	struct LockCertificate {
		static int Action(ssa::server::SsaSession* session, ssa::Publisher::Messages::ContainerOperation::LockCertificate* msg) {
			return E_SUCCESS;
		}
	};

	ActionFunction action_[8];
};


class LockVerifier {
public:
	int VerifyLock(::ssa::server::SsaSession* session, ssa::common::ObjectId oid);

};



int
Verifier::Parse(::ssa::server::SsaSession* session, 
                ::ssa::Publisher::Messages::BaseMessage* next)
{
	char                                                buf[512];
	ssa::Publisher::Messages::BaseMessage*              msg;
	ssa::Publisher::Messages::ContainerOperationHeader* header;
	ssa::server::SsaSharedBuffer*                       shbuf = session->shbuf_;

	while (shbuf->Read(buf, sizeof(*msg))) {
		msg = ssa::Publisher::Messages::BaseMessage::Load(buf);
		if (msg->type_ != ssa::Publisher::Messages::kContainerOperation) {
			*next = *msg;
			break;
		}
		if (shbuf->Read(&buf[sizeof(*msg)], sizeof(*header) - sizeof(*msg)) <
		    (sizeof(*header) - sizeof(*msg))) {
			return -1;
		}
		header = ssa::Publisher::Messages::ContainerOperationHeader::Load(buf);
		action_[header->id_](session, buf, header);
	}
	return 1; /* there is one next message  */
}


#define PARSER_ADD_ACTION(__physical_op)                                                    \
    action_[ssa::Publisher::Messages::ContainerOperation::k##__physical_op] =                \
        Action<ssa::Publisher::Messages::ContainerOperation::__physical_op, __physical_op>;

} // namespace server

#endif // __STAMNOS_SSA_SERVER_CONTAINER_VERIFIER_H
