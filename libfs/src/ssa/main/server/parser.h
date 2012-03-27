/**
 * \brief Parses a stream of physical_operations
 */

#ifndef __STAMNOS_SSA_SERVER_STREAM_PARSER_H
#define __STAMNOS_SSA_SERVER_STREAM_PARSER_H

typedef int (*ActionFunction)(ssa::server::SsaSharedBuffer*, char*, ssa::Publisher::Messages::PhysicalOperationHeader*);

class Parser {
public:
	int Parse(::ssa::server::SsaSession* session, char* lgc_op_hdr, ::ssa::Publisher::Messages::BaseMessage* next);

	ActionFunction action_[8];
};


template<typename T, typename A>
int Action(ssa::server::SsaSharedBuffer* shbuf, 
           char* buf, 
           ssa::Publisher::Messages::PhysicalOperationHeader* header) 
{
	size_t size = sizeof(T);
	T* physical_op_msg;
	if (size > sizeof(*header)) {
		shbuf->Read(&buf[sizeof(*header)], size - sizeof(*header));
		physical_op_msg = T::Load(buf);
	}
	return A::Action(physical_op_msg);
};


int
Parser::Parse(::ssa::server::SsaSession* session, 
              char* lgc_op_hdr, 
              ::ssa::Publisher::Messages::BaseMessage* next)
{
	char                                               buf[512];
	ssa::Publisher::Messages::BaseMessage*             msg;
	ssa::Publisher::Messages::PhysicalOperationHeader* header;
	ssa::server::SsaSharedBuffer*                      shbuf = session->shbuf_;

	while (shbuf->Read(buf, sizeof(*msg))) {
		msg = ssa::Publisher::Messages::BaseMessage::Load(buf);
		if (msg->type_ != ssa::Publisher::Messages::kPhysicalOperation) {
			*next = *msg;
			break;
		}
		if (shbuf->Read(&buf[sizeof(*msg)], sizeof(*header) - sizeof(*msg)) <
		    (sizeof(*header) - sizeof(*msg))) {
			return -1;
		}
		header = ssa::Publisher::Messages::PhysicalOperationHeader::Load(buf);
		printf("PHYSICAL_OPERATION: %d\n", header->id_);
		action_[header->id_](shbuf, buf, header);
	}
	return E_SUCCESS;
}


#define PARSER_ADD_ACTION(__physical_op)                                                    \
    action_[ssa::Publisher::Messages::PhysicalOperation::k##__physical_op] =                \
        Action<ssa::Publisher::Messages::PhysicalOperation::__physical_op, __physical_op>;


#endif // __STAMNOS_SSA_SERVER_STREAM_PARSER_H
