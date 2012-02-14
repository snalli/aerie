#include "dpo/main/server/registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <cstring>
#include "common/errno.h"
#include "ipc/ipc.h"
#include "dpo/main/common/registry_protocol.h"

// TODO: make the registry persistent

namespace dpo {
namespace server {


Registry::Registry(::server::Ipc* ipc)
	: ipc_(ipc),
	  ipc_handlers_(this)
{
	int ret; 
	ret = pthread_mutex_init(&mutex_, NULL);
	assert(ret == 0);
}


int Registry::Init()
{
	return ipc_handlers_.Init();
}


int Registry::Lookup(std::string name, uint64_t* obj)
{
	int                                       i;
	std::map<std::string, uint64_t>::iterator it;

	pthread_mutex_lock(&mutex_);
	it = map_.find(name);
	if (it == map_.end()) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	*obj = it->second;
	pthread_mutex_unlock(&mutex_);
	return 0;
}


int Registry::Add(std::string name, uint64_t obj)
{
	int                                                        i;
	std::pair<std::map<std::string, uint64_t>::iterator, bool> pairret;
	pthread_mutex_lock(&mutex_);
	pairret = map_.insert(std::pair<std::string, uint64_t>(name, obj));
	if (pairret.second != true) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	pthread_mutex_unlock(&mutex_);
	return 0;
}


int Registry::Remove(std::string name)
{
	int ret;

	pthread_mutex_lock(&mutex_);
	ret = map_.erase(name);
	pthread_mutex_unlock(&mutex_);

	return ret;
}


int 
Registry::IpcHandlers::Init()
{
	registry_->ipc_->reg(::dpo::RegistryProtocol::kLookup, this, 
	          &::dpo::server::Registry::IpcHandlers::Lookup);
//	ipc_->reg(::dpo::RegistryProtocol::kAdd, this, 
//	          &::dpo::server::Registry::IPC::Add);
//	ipc_->reg(::dpo::RegistryProtocol::kRemove, this, 
//	          &::dpo::server::Registry::IPC::Remove);
	
	return E_SUCCESS;
}



int
Registry::IpcHandlers::Lookup(int clt, const std::string name, unsigned long long &r)
{
    int       ret;
    uint64_t  val;

    ret = registry_->Lookup(name, &val);
    if (ret<0) {
        return -ret;
    }
    r = (unsigned long long) val;
    return 0;
}


} // namespace server
} // namespace dpo
