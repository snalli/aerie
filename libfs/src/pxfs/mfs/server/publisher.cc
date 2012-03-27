#include "pxfs/mfs/server/publisher.h"
#include "ssa/main/server/stsystem.h"
#include "ssa/main/server/session.h"
#include "ssa/main/server/shbuf.h"
#include "ssa/main/server/parser.h"
#include "common/errno.h"

namespace server {

int
Publisher::Register(::ssa::server::StorageSystem* stsystem)
{
	stsystem->publisher()->RegisterOperation(1, Publisher::Write);
	return E_SUCCESS;
}


class WriteParser: public Parser {
public:

	struct AllocateExtent {
		static int Action(ssa::Publisher::Messages::PhysicalOperation::AllocateExtent* msg) {
			return E_SUCCESS;
		}
	};
	
	struct LinkBlock {
		static int Action(ssa::Publisher::Messages::PhysicalOperation::LinkBlock* msg) {
			printf("LINK_BLOCK\n");
			return E_SUCCESS;
		}
	};

	WriteParser()
	{
		PARSER_ADD_ACTION(AllocateExtent);
		PARSER_ADD_ACTION(LinkBlock);
	}
};


int
Publisher::Write(::ssa::server::SsaSession* session, char* lgc_op_hdr, 
                 ::ssa::Publisher::Messages::BaseMessage* next)
{
	printf("WRITE\n");
	return write_parser_->Parse(session, lgc_op_hdr, next);
}


int
Publisher::Init()
{
	write_parser_ = new WriteParser();
	return E_SUCCESS;
}


WriteParser* Publisher::write_parser_ = NULL;

} // namespace server
