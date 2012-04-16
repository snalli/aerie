#ifndef __STAMNOS_KVFS_SERVER_H
#define __STAMNOS_KVFS_SERVER_H

#include "osd/osd-opaque.h"
#include "bcs/bcs-opaque.h"

namespace server {

class FileSystem; // forward declaration

class Server {
public:
	static Server* Instance();
	void Init(const char* pathname, int flags, int port);
	void Start();

	Ipc* ipc_layer() { return ipc_layer_; }

private:
	int                          port_;
	Ipc*                         ipc_layer_;
	FileSystem*                  fs_;
	static Server*               instance_;
};


} // namespace server

#endif // __STAMNOS_KVFS_SERVER_H
