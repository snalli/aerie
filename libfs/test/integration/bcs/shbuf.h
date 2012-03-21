#include "bcs/main/server/shbuf.h"
#include "bcs/main/client/shbuf.h"

namespace server {

class TestSharedBuffer: public SharedBuffer {
public:
	static SharedBuffer* Make() {
		return new TestSharedBuffer();
	}
};

} // namespace server 
