#define  __CACHE_GUARD__

#include "cfs/server/session.h"
#include "osd/main/server/osd.h"
#include "osd/main/server/session.h"
#include "common/errno.h"


namespace server {

int 
Session::Init(int clt) 
{
	osd::server::OsdSession::Init(clt);
	return E_SUCCESS;
}


} // namespace server
