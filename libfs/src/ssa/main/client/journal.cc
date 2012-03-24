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
	LogicalOpHeader header = LogicalOpHeader(id);
	buffer_.Write64((uint64_t*) &header);
	return E_SUCCESS;
}


int
Journal::EndLogicalOperation()
{
	CommandHeader header = CommandHeader(0);
	buffer_.Write64((uint64_t*) &header);
	buffer_.Flush(session_->stsystem()->shbuf());
	return E_SUCCESS;
}


} // namespace client
} // namespace ssa
