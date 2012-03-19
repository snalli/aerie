#include "bcs/main/server/cltdsc.h"
#include <stdio.h>
#include "bcs/main/server/shbuf.h"
#include "common/errno.h"

namespace server {

int
ClientDescriptor::Init()
{
	int  ret;
	char buf[64];

	if ((shbuf_ = new SharedBuffer()) == NULL) {
		return -E_NOMEM;
	}
	sprintf(buf, "%d", clt_);

	if ((ret = shbuf_->Init(buf)) < 0) {
		return ret;
	}
	return E_SUCCESS;
}


} // namespace server
