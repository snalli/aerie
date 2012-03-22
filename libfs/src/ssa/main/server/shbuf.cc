#include "ssa/main/server/shbuf.h"
#include <stdio.h>

namespace ssa {
namespace server {

int SsaSharedBuffer::Consume()
{
	printf("CONSUME\n");
}

} // namespace server
} // namespace ssa
