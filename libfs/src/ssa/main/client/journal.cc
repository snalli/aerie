#include "ssa/main/client/journal.h"
#include "ssa/main/client/session.h"
#include "ssa/main/common/publisher.h"

namespace ssa {
namespace client {

typedef ::ssa::Publisher::Messages::TransactionBegin        TransactionBeginMessage;
typedef ::ssa::Publisher::Messages::TransactionEnd          TransactionEndMessage;
typedef ::ssa::Publisher::Messages::LogicalOperationHeader  LogicalOperationHeaderMessage;
typedef ::ssa::Publisher::Messages::PhysicalOperationHeader PhysicalOperationHeaderMessage;


int
Journal::TransactionBegin(int id)
{
	printf("TRANSACTION_BEGIN\n");
	TransactionBeginMessage tx = TransactionBeginMessage(id);
	buffer_.Write(&tx, sizeof(tx));
	return E_SUCCESS;
}


int
Journal::TransactionEnd()
{
	printf("TRANSACTION_END\n");
	TransactionEndMessage tx = TransactionEndMessage();
	buffer_.Write(&tx, sizeof(tx));
	buffer_.Flush(session_->stsystem()->shbuf());
	return E_SUCCESS;
}


int
Journal::LogicalOperation(int id)
{
	printf("LOGICAL_OPERATION(%d)\n", id);
	LogicalOperationHeaderMessage header = LogicalOperationHeaderMessage(id);
	buffer_.Write(&header, sizeof(header));
	return E_SUCCESS;
}



} // namespace client
} // namespace ssa
