#include "bcs/main/server/shbufmgr.h"
#include <map>
#include "bcs/main/server/ipc.h"
#include "bcs/main/server/session.h"
#include "bcs/main/common/config.h"
#include "bcs/main/common/shbuf_protocol.h"
#include "common/util.h"
#include "common/errno.h"

namespace server {

SharedBufferManager::SharedBufferManager(Ipc* ipc)
	: ipc_(ipc)
{ }


int
SharedBufferManager::Init()
{
	int ret;

	if (ipc_) {
		if ((ret = ipc_handlers_.Register(this)) < 0) {
			return ret;
		}
	}
	return runtime_config_.Init();
}


int
SharedBufferManager::RegisterSharedBufferType(const char* buffertypeid, 
                                              SharedBufferCreator creator)
{
	types_[buffertypeid] = creator;
	return E_SUCCESS;
}


/**
 * \brief Creates and assigns a shared buffer to the current client
 */
SharedBuffer*
SharedBufferManager::CreateSharedBuffer(const char* buffertypeid, 
                                        BcsSession* session)
{
	SharedBuffer*       buf;
	SharedBufferCreator creator = types_[buffertypeid];
	if ((buf = creator()) == NULL) {
		return NULL;
	}
	session->shbuf_vec_.push_back(buf);
	return buf;
}


int
SharedBufferManager::Consume(BcsSession* session, int id, int& r) 
{
	
	return E_SUCCESS;
}


/*
 * RUNTIME CONFIGURATION 
 */

size_t SharedBufferManager::RuntimeConfig::sharedbuffer_size;

int
SharedBufferManager::RuntimeConfig::Init()
{
	int   ret;
	char* csize;

	if ((ret = Config::Lookup("ipc.sharedbuffer.size", &csize)) < 0) {
		return ret;
	}
	sharedbuffer_size = StringToSize(csize);
	
	return E_SUCCESS;
}



/*
 * IPC HANDLERS 
 */

int
SharedBufferManager::IpcHandlers::Register(SharedBufferManager* manager)
{
	manager_ = manager;
    manager_->ipc_->reg(SharedBufferProtocol::kConsume, this, 
	                    &SharedBufferManager::IpcHandlers::Consume);

	return E_SUCCESS;
}


int
SharedBufferManager::IpcHandlers::Consume(int clt, int id, int& r)
{
	int          ret;
	BaseSession* session;

	if ((ret = manager_->ipc_->session_manager()->Lookup(clt, &session)) < 0) {
		return -ret;
	}
	if ((ret = manager_->Consume(static_cast<BcsSession*>(session), id, r)) < 0) {
		return -ret;
	}
	return E_SUCCESS;
}



} // namespace server
