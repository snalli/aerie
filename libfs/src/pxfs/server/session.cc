#include "pxfs/server/session.h"
#include "common/errno.h"
#include "osd/main/server/osd.h"
#include "osd/main/server/session.h"

namespace server
{

int Session::Init(int clt)
{
    osd::server::OsdSession::Init(clt);
    return E_SUCCESS;
}

} // namespace server
