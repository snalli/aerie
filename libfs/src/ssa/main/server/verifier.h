/**
 * \brief Parses and verifies a stream of container_operations
 */

#ifndef __STAMNOS_SSA_SERVER_CONTAINER_VERIFIER_H
#define __STAMNOS_SSA_SERVER_CONTAINER_VERIFIER_H

typedef int (*ActionFunction)(ssa::server::SsaSession* session, char* buf, ssa::Publisher::Messages::ContainerOperationHeader*);

class Verifier {
public:
	int Parse(::ssa::server::SsaSession* session, ::ssa::Publisher::Messages::BaseMessage* next);

	ActionFunction action_[8];
};


template<typename T, typename A>
int Action(ssa::server::SsaSession* session, 
           char* buf, 
           ssa::Publisher::Messages::ContainerOperationHeader* header) 
{
	ssa::server::SsaSharedBuffer* shbuf = session->shbuf_;
	size_t                        size = sizeof(T);
	T* physical_op_msg;
	if (size > sizeof(*header)) {
		shbuf->Read(&buf[sizeof(*header)], size - sizeof(*header));
		physical_op_msg = T::Load(buf);
	}
	return A::Action(session, physical_op_msg);
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
		printf("PHYSICAL_OPERATION: %d\n", header->id_);
		action_[header->id_](session, buf, header);
	}
	return E_SUCCESS;
}


#define PARSER_ADD_ACTION(__physical_op)                                                    \
    action_[ssa::Publisher::Messages::ContainerOperation::k##__physical_op] =                \
        Action<ssa::Publisher::Messages::ContainerOperation::__physical_op, __physical_op>;


#endif // __STAMNOS_SSA_SERVER_CONTAINER_VERIFIER_H
