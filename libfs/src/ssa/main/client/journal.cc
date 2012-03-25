#include "ssa/main/client/journal.h"
#include "ssa/main/client/session.h"
#include "ssa/main/common/publisher.h"

namespace ssa {
namespace client {

typedef ::ssa::Publisher::Messages::LogicalOpHeader LogicalOpHeader;
typedef ::ssa::Publisher::Messages::CommandHeader CommandHeader;

int
Journal::BeginLogicalOperation(int id)
{
	printf("BEGIN_LOGICAL_OPERATION(%d)\n", id);
	LogicalOpHeader header = LogicalOpHeader(id);
	buffer_.Write64((uint64_t*) &header);
	return E_SUCCESS;
}


int
Journal::EndLogicalOperation()
{
	printf("END_LOGICAL_OPERATION\n");
	CommandHeader header = CommandHeader(0);
	buffer_.Write64((uint64_t*) &header);
	buffer_.Flush(session_->stsystem()->shbuf());
	return E_SUCCESS;
}


int 
Journal::Command(ssa::Publisher::Messages::CommandHeader* cmd, size_t size)
{
	printf("COMMAND: %d %d\n", cmd->id_, size);
	buffer_.Write((uint64_t*) cmd, size);
	return E_SUCCESS;
}


} // namespace client
} // namespace ssa
