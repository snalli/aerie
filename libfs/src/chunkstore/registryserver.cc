#include "registryserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <cstring>
#include "common/uthash.h"


RegistryServer::RegistryServer()
{
	int ret; 
	ret = pthread_mutex_init(&mutex_, NULL);
	assert(ret == 0);
}


int RegistryServer::Init()
{

}


int RegistryServer::Lookup(std::string name, uint64_t* obj)
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


int RegistryServer::Add(std::string name, uint64_t obj)
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


int RegistryServer::Remove(std::string name)
{
	int ret;

	pthread_mutex_lock(&mutex_);
	ret = map_.erase(name);
	pthread_mutex_unlock(&mutex_);

	return ret;
}
