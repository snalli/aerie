#include "bcs/main/server/cltdsc.h"
#include "bcs/main/server/shbuf.h"
#include "common/errno.h"

namespace server {

int
ClientDescriptor::Init()
{
	int ret;

	if ((shbuf_ = new SharedBuffer()) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = shbuf_->Init()) < 0) {
		return ret;
	}
	return E_SUCCESS;
}


} // namespace server
