#ifndef __STAMNOS_BCS_SERVER_SHARED_BUFFER_MANAGER_H
#define __STAMNOS_BCS_SERVER_SHARED_BUFFER_MANAGER_H

#include <stddef.h>
#include <string>
#include <map>
#include "bcs/main/server/shbuf.h"
#include "bcs/main/server/ipc.h"
#include "bcs/main/server/bcs-opaque.h"


namespace server {

class SharedBufferManager {
	typedef SharedBuffer* (*SharedBufferCreator)();

public:
	SharedBufferManager(Ipc* ipc);
	int Init();
	int RegisterSharedBufferType(const char* buffertypeid, SharedBufferCreator creator);
	SharedBuffer* CreateSharedBuffer(const char* buffertypeid, BcsSession* session);
	int Consume(BcsSession* session, int id, int& r);


	class RuntimeConfig {
	public:
		static int Init();

		static size_t sharedbuffer_size;
	};

	class IpcHandlers {
	public:
		int Register(SharedBufferManager* manager);

		int Consume(int clt, int id, int& r);

	private:
		SharedBufferManager* manager_;
	}; 
private:
	std::map<std::string, SharedBufferCreator> types_; // registered buffer types
	Ipc*                                       ipc_;
	IpcHandlers                                ipc_handlers_;
	RuntimeConfig                              runtime_config_;
}; 

} // namespace server

#endif // __STAMNOS_BCS_SERVER_SHARED_BUFFER_MANAGER_H
