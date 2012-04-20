#include "osd/main/server/journal.h"
#include "common/errno.h"

namespace osd {
namespace server {

int
Journal::TransactionBegin(int id)
{
	return E_SUCCESS;
}


int
Journal::TransactionCommit()
{
	return E_SUCCESS;
}


} // namespace server
} // namespace osd
