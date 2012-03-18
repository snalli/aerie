#ifndef __STAMNOS_BCS_SERVER_SHARED_BUFFER_H
#define __STAMNOS_BCS_SERVER_SHARED_BUFFER_H

#include <stddef.h>
#include <string>

namespace server {

class SharedBuffer {
public:
	int Init();

private:
	void*       base_;
	size_t      size_;
	std::string path_;
};

} // namespace server

#endif // __STAMNOS_BCS_SERVER_SHARED_BUFFER_H
