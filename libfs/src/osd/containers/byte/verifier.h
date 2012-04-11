#ifndef __STAMNOS_OSD_BYTE_CONTAINER_VERIFIER_H
#define __STAMNOS_OSD_BYTE_CONTAINER_VERIFIER_H

#include "osd/main/server/verifier.h"
#include "osd/containers/byte/container.h"

namespace server {

class WriteVerifier: public Verifier {
public:

	struct AllocateExtent {
		static int Action(osd::server::OsdSession* session, osd::Publisher::Message::ContainerOperation::AllocateExtent* msg) {
			return E_SUCCESS;
		}
	};
	
	struct LinkBlock {
		static int Action(osd::server::OsdSession* session, osd::Publisher::Message::ContainerOperation::LinkBlock* msg) {
			osd::common::ObjectId& oid = msg->oid_;
			osd::containers::server::ByteContainer::Object* object = osd::containers::server::ByteContainer::Object::Load(oid);
			object->LinkBlock(session, msg->bn_, msg->ptr_);
			return E_SUCCESS;
		}
	};

	WriteVerifier()
	{
		PARSER_ADD_ACTION(AllocateExtent);
		PARSER_ADD_ACTION(LinkBlock);
		PARSER_ADD_ACTION(LockCertificate);
	}
};

} // namespace server

#endif // __STAMNOS_OSD_BYTE_CONTAINER_VERIFIER_H
