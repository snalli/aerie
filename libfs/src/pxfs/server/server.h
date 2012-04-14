#ifndef __STAMNOS_PXFS_SERVER_H
#define __STAMNOS_PXFS_SERVER_H

#include "osd/osd-opaque.h"
#include "bcs/bcs-opaque.h"

namespace server {

class FileSystem; // forward declaration

class Server {
public:
	static Server* Instance();
	void Start(const char* pathname, int flags, int port);

	Ipc* ipc_layer() { return ipc_layer_; }

private:
	int                          port_;
	Ipc*                         ipc_layer_;
	FileSystem*                  fs_;
	static Server*               instance_;
};


} // namespace server

#endif // __STAMNOS_PXFS_SERVER_H
