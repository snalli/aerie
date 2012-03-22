#include "ssa/main/server/shbuf.h"
#include <stdio.h>

namespace ssa {
namespace server {

int SsaSharedBuffer::Consume()
{
	printf("CONSUME: %d\n", header_->end_);
}

} // namespace server
} // namespace ssa
