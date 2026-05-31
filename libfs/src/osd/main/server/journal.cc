#include "osd/main/server/journal.h"
#include "common/errno.h"
#include "scm/scm/model.h"

namespace osd
{
namespace server
{

int Journal::TransactionBegin(int /*id*/)
{
    return E_SUCCESS;
}

int Journal::TransactionCommit()
{
    ScmFence();
    return E_SUCCESS;
}

int Journal::TransactionAbort()
{
    return E_SUCCESS;
}

} // namespace server
} // namespace osd
