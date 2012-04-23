#ifndef __STAMNOS_OSD_BYTE_CONTAINER_VERIFIER_H
#define __STAMNOS_OSD_BYTE_CONTAINER_VERIFIER_H

#include "osd/main/server/verifier.h"
#include "osd/containers/byte/container.h"

namespace server {

class WriteVerifier: public Verifier {
public:

	struct AllocateExtent {
		static int Action(osd::server::OsdSession* session, osd::Publisher::Message::ContainerOperation::AllocateExtent* msg) {
			osd::common::ObjectId set_oid;
			set_oid = session->sets_[msg->capability_];
			session->salloc()->AllocateExtentFromSet(session, set_oid, msg->eid_, msg->index_hint_);
			return E_SUCCESS;
		}
	};
	
	struct LinkBlock {
		static int Action(osd::server::OsdSession* session, osd::Publisher::Message::ContainerOperation::LinkBlock* msg) {
			osd::common::ObjectId& oid = msg->oid_;
			osd::containers::server::ByteContainer::Object* object = osd::containers::server::ByteContainer::Object::Load(oid);
			printf("LINK: %lu -> %p\n", msg->bn_, msg->ptr_);
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
