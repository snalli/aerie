#include "osd/main/client/journal.h"
#include "osd/main/client/session.h"
#include "osd/main/common/publisher.h"

namespace osd {
namespace client {

typedef ::osd::Publisher::Message::TransactionBegin         TransactionBeginMessage;
typedef ::osd::Publisher::Message::TransactionCommit        TransactionCommitMessage;
typedef ::osd::Publisher::Message::LogicalOperationHeader   LogicalOperationHeaderMessage;
typedef ::osd::Publisher::Message::ContainerOperationHeader ContainerOperationHeaderMessage;


int
Journal::TransactionBegin(int id)
{
	TransactionBeginMessage tx = TransactionBeginMessage(id);
	buffer_.Write(&tx, sizeof(tx));
	return E_SUCCESS;
}


int
Journal::TransactionCommit()
{
	TransactionCommitMessage tx = TransactionCommitMessage();
	buffer_.Write(&tx, sizeof(tx));
	buffer_.Flush(session_->stsystem()->shbuf());
	return E_SUCCESS;
}


} // namespace client
} // namespace osd
