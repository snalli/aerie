#include "osd/main/server/journal.h"
#include "scm/scm/model.h"
#include "common/errno.h"
#include "osd/main/server/stats.h"

namespace osd {
namespace server {

int
Journal::TransactionBegin(int id)
{
	nentries_ = 0;
	return E_SUCCESS;
}

int
Journal::TransactionCommit()
{
	ScmFence();
	for (int i=0; i < nentries_; i++) {
		ScmFlush(entries_[i].addr);
		STATISTICS_INC(clflushes);
	}
	ScmFence();
	nentries_ = 0;
	STATISTICS_INC(txcommits);
	return E_SUCCESS;
}

int
Journal::TransactionAbort()
{
	return E_SUCCESS;
}



} // namespace server
} // namespace osd
