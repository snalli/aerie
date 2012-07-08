#ifndef __STAMNOS_OSD_BYTE_CONTAINER_VERIFIER_H
#define __STAMNOS_OSD_BYTE_CONTAINER_VERIFIER_H

#include "osd/main/server/verifier.h"
#include "osd/containers/byte/container.h"

namespace server {

typedef struct {
    uid_t uid;
    int rw;
} user_file_rights;

typedef struct {
	unsigned long base;
	unsigned long size;
} KernelExtent;



static int
Protect(unsigned long extent_base, size_t extent_size, uid_t uid, int rw)
{
	int              ret;
	KernelExtent     e;
	user_file_rights r;

	e.base = extent_base;
	e.size = extent_size; // convention: size in bytes
	r.uid = uid;
	r.rw = rw;

	// convention: syscall 313 returns 1 on success
	if (syscall(313, (void*) &e, (void*) &r, 1) != 1) {
		return -E_PROT;
	}
	return E_SUCCESS;
}



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
			object->LinkBlock(session, msg->bn_, msg->ptr_);
			if ((uint64_t) object == 0x80fffc0700) {
				printf("%llu %p\n", msg->bn_, msg->ptr_);
				if (msg->bn_ == 9) {
					//printf("CHANGE PROTECTION\n");
					 //Protect(0x80faa81000, 4096, getuid(), 0x2);
				}
				object->PrintBlocks(session);
			}
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
