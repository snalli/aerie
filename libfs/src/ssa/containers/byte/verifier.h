#ifndef __STAMNOS_SSA_BYTE_CONTAINER_VERIFIER_H
#define __STAMNOS_SSA_BYTE_CONTAINER_VERIFIER_H

#include "ssa/main/server/verifier.h"
#include "ssa/containers/byte/container.h"

namespace server {

class WriteVerifier: public Verifier {
public:

	struct AllocateExtent {
		static int Action(ssa::server::SsaSession* session, ssa::Publisher::Messages::ContainerOperation::AllocateExtent* msg) {
			return E_SUCCESS;
		}
	};
	
	struct LinkBlock {
		static int Action(ssa::server::SsaSession* session, ssa::Publisher::Messages::ContainerOperation::LinkBlock* msg) {
			ssa::common::ObjectId& oid = msg->oid_;
			ssa::containers::server::ByteContainer::Object* object = ssa::containers::server::ByteContainer::Object::Load(oid);
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

#endif // __STAMNOS_SSA_BYTE_CONTAINER_VERIFIER_H
