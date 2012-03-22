#include "bcs/main/server/shbuf.h"
#include "bcs/main/client/shbuf.h"
#include "bcs/main/server/bcs-opaque.h"

namespace server {

class TestSharedBuffer: public SharedBuffer {
public:
	static SharedBuffer* Make() {
		return new TestSharedBuffer();
	}
	int Consume(BcsSession* session) { }
};

} // namespace server 
