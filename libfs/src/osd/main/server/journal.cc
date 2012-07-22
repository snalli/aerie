#include "osd/main/server/journal.h"
#include "scm/scm/model.h"
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
	ScmFence();
	return E_SUCCESS;
}

int
Journal::TransactionAbort()
{
	return E_SUCCESS;
}



} // namespace server
} // namespace osd
