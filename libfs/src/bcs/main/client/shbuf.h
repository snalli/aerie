#ifndef __STAMNOS_BCS_CLIENT_SHARED_BUFFER_H
#define __STAMNOS_BCS_CLIENT_SHARED_BUFFER_H

#include <stddef.h>
#include <string>
#include "bcs/main/common/shbuf.h"

namespace client {

class SharedBuffer {
public:
	SharedBuffer(SharedBufferDescriptor& dsc)
		: size_(dsc.size_),
		  path_(dsc.path_)
	{ }

	int Map();

private:
	void*       base_;
	size_t      size_;
	std::string path_;
};

} // namespace client

#endif // __STAMNOS_BCS_CLIENT_SHARED_BUFFER_H
