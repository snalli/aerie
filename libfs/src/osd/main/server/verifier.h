/**
 * \brief Parses and verifies a stream of container_operations
 */

#ifndef __STAMNOS_OSD_SERVER_CONTAINER_VERIFIER_H
#define __STAMNOS_OSD_SERVER_CONTAINER_VERIFIER_H

#include "osd/main/common/obj.h"
#include "osd/main/server/session.h"
#include "osd/main/common/publisher.h"

typedef int (*ActionFunction)(osd::server::OsdSession* session, char* buf, osd::Publisher::Message::ContainerOperationHeader*);

template<typename T, typename A>
int Action(osd::server::OsdSession* session, 
           char* buf, 
           osd::Publisher::Message::ContainerOperationHeader* header) 
{
	osd::server::OsdSharedBuffer* shbuf = session->shbuf_;
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
	int Parse(::osd::server::OsdSession* session, ::osd::Publisher::Message::BaseMessage* next);
	
	struct LockCertificate {
		static int Action(osd::server::OsdSession* session, osd::Publisher::Message::ContainerOperation::LockCertificate* msg) {
			return E_SUCCESS;
		}
	};

	ActionFunction action_[8];
};


class LockVerifier {
public:
	int VerifyLock(::osd::server::OsdSession* session, osd::common::ObjectId oid);

};



int
Verifier::Parse(::osd::server::OsdSession* session, 
                ::osd::Publisher::Message::BaseMessage* next)
{
	char                                                buf[512];
	osd::Publisher::Message::BaseMessage*              msg;
	osd::Publisher::Message::ContainerOperationHeader* header;
	osd::server::OsdSharedBuffer*                       shbuf = session->shbuf_;

	while (shbuf->Read(buf, sizeof(*msg))) {
		msg = osd::Publisher::Message::BaseMessage::Load(buf);
		if (msg->type_ != osd::Publisher::Message::kContainerOperation) {
			*next = *msg;
			break;
		}
		if (shbuf->Read(&buf[sizeof(*msg)], sizeof(*header) - sizeof(*msg)) <
		    (sizeof(*header) - sizeof(*msg))) {
			return -1;
		}
		header = osd::Publisher::Message::ContainerOperationHeader::Load(buf);
		action_[header->id_](session, buf, header);
	}
	return 1; /* there is one next message  */
}


#define PARSER_ADD_ACTION(__physical_op)                                                    \
    action_[osd::Publisher::Message::ContainerOperation::k##__physical_op] =                \
        Action<osd::Publisher::Message::ContainerOperation::__physical_op, __physical_op>;

} // namespace server

#endif // __STAMNOS_OSD_SERVER_CONTAINER_VERIFIER_H
