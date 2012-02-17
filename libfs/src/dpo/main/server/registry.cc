#include "dpo/main/server/registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <cstring>
#include "common/errno.h"
#include "ipc/ipc.h"
#include "dpo/main/common/obj.h"
#include "dpo/main/common/registry_protocol.h"

// TODO: make the registry persistent

namespace dpo {
namespace server {


Registry::Registry(::server::Ipc* ipc)
	: ipc_(ipc)
{
	int ret; 
	ret = pthread_mutex_init(&mutex_, NULL);
	assert(ret == 0);
}


int 
Registry::Init()
{
	return ipc_handlers_.Register(this);
}


int 
Registry::Lookup(std::string name, ::dpo::common::ObjectId* oid)
{
	int                                                      i;
	std::map<std::string, ::dpo::common::ObjectId>::iterator it;
	
	pthread_mutex_lock(&mutex_);
	it = map_.find(name);
	if (it == map_.end()) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	*oid = it->second;
	pthread_mutex_unlock(&mutex_);
	return 0;
}


int 
Registry::Add(std::string name, ::dpo::common::ObjectId oid)
{
	int                                                                       i;
	std::pair<std::map<std::string, ::dpo::common::ObjectId>::iterator, bool> pairret;
	pthread_mutex_lock(&mutex_);
	pairret = map_.insert(std::pair<std::string, ::dpo::common::ObjectId>(name, oid));
	if (pairret.second != true) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	pthread_mutex_unlock(&mutex_);
	return 0;
}


int 
Registry::Remove(std::string name)
{
	int ret;

	pthread_mutex_lock(&mutex_);
	ret = map_.erase(name);
	pthread_mutex_unlock(&mutex_);

	return ret;
}


int 
Registry::IpcHandlers::Register(Registry* registry)
{
	registry_ = registry;	
	registry_->ipc_->reg(::dpo::RegistryProtocol::kLookup, this, 
	                     &::dpo::server::Registry::IpcHandlers::Lookup);
	registry_->ipc_->reg(::dpo::RegistryProtocol::kAdd, this, 
	                     &::dpo::server::Registry::IpcHandlers::Add);
	registry_->ipc_->reg(::dpo::RegistryProtocol::kRemove, this, 
   	                     &::dpo::server::Registry::IpcHandlers::Remove);
	
	return E_SUCCESS;
}


int
Registry::IpcHandlers::Lookup(unsigned int clt, const std::string name, 
                              ::dpo::common::ObjectId &r)
{
    int                     ret;
	::dpo::common::ObjectId tmp_oid;

    if ((ret = registry_->Lookup(name, &tmp_oid)) < 0) {
		return -ret;
	}
	r = tmp_oid;
    return 0;
}


int
Registry::IpcHandlers::Add(unsigned int clt, const std::string name, 
                           ::dpo::common::ObjectId oid, int &r)
{
    int ret;

    if ((ret = registry_->Add(name, oid)) < 0) {
        return -ret;
	}
    return 0;
}


int
Registry::IpcHandlers::Remove(unsigned int clt, const std::string name, int &r)
{
    int ret;

    if ((ret = registry_->Remove(name)) < 0) {
		return -ret;
	}
    return 0;
}


} // namespace server
} // namespace dpo
