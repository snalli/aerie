#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>
#include <iostream>
#include <vector>
#include "rpc/rpc.h"
#include "rpc/jsl_log.h"
#include "chunkstore/chunkserver.h"
#include "chunkstore/registryserver.h"
#include "common/debug.h"
#include "server/lckmgr.h"
#include "api.h"

using namespace server;

rpcs*           serverp;  // server rpc object
int             port;
pthread_attr_t  attr;
ChunkServer*    chunk_server;
RegistryServer* registry;
LockManager*    lm;


// server-side handlers. they must be methods of some class
// to simplify rpcs::reg(). a server process can have handlers
// from multiple classes.
class srv {
	public:
		int alive(const unsigned int principal_id, int& r);
		int chunk_create(const unsigned int principal_id, const unsigned long long size, const int type, unsigned long long& r);
		int chunk_delete(const unsigned int principal_id, unsigned long long chunkdsc, int & r);
		int chunk_access(const unsigned int principal_id, std::vector<unsigned long long> vuchunkdsc, unsigned int, int& r);
		int chunk_release(const unsigned int principal_id, std::vector<unsigned long long> vuchunkdsc, int& r);
        int name_lookup(const unsigned int principal_id, const std::string name, unsigned long long& r);
        int name_link(const unsigned int principal_id, const std::string name, unsigned long long inode, int& r);
		int name_unlink(const unsigned int principal_id, const std::string name, int &r);
        int registry_lookup(const unsigned int principal_id, const std::string name, unsigned long long& obj);
        int registry_add(const unsigned int principal_id, const std::string name, unsigned long long obj, int& r);
		int registry_remove(const unsigned int principal_id, const std::string name, int &r);
        int namespace_mount(const unsigned int principal_id, const std::string name, unsigned long long superblock, int& r);
};


int 
srv::alive(const unsigned int principal_id, int& r)
{
	r = 0;

	return 0;
}


int
srv::chunk_create(const unsigned int principal_id, const unsigned long long size, const int type, unsigned long long& r)
{
	ChunkDescriptor*   chunkdsc;
	unsigned long long chunkdsc_id;
	int                ret;

	ret = chunk_server->CreateChunk(principal_id, size, type, &chunkdsc);
	if (ret == 0) {
		chunkdsc_id = (unsigned long long) chunkdsc;
		r = chunkdsc_id;
	} else {
		r = 0;
	}
	return 0;
}


int
srv::chunk_delete(const unsigned int principal_id, const unsigned long long chunkdsc_id, int &r)
{
	ChunkDescriptor* chunkdsc = (ChunkDescriptor*) chunkdsc_id;
	r = chunk_server->DeleteChunk(principal_id, chunkdsc);
	return 0;
}


int
srv::chunk_access(const unsigned int principal_id, std::vector<unsigned long long> vuchunkdsc, unsigned int prot_flags, int &r)
{
	std::vector<unsigned long long>::iterator it;
	std::vector<ChunkDescriptor*>             vchunkdsc;
	unsigned long long                        uchunkdsc;

	for (it = vuchunkdsc.begin(); it != vuchunkdsc.end(); it++) {
		uchunkdsc = *it;
		vchunkdsc.push_back((ChunkDescriptor*) uchunkdsc);
	}
	r = chunk_server->AccessChunk(principal_id, vchunkdsc, prot_flags);
	return 0;
}


int
srv::chunk_release(const unsigned int principal_id, std::vector<unsigned long long> vuchunkdsc, int &r)
{
	std::vector<unsigned long long>::iterator it;
	std::vector<ChunkDescriptor*>             vchunkdsc;
	unsigned long long                        uchunkdsc;

	for (it = vuchunkdsc.begin(); it != vuchunkdsc.end(); it++) {
		uchunkdsc = *it;
		vchunkdsc.push_back((ChunkDescriptor*) uchunkdsc);
	}
	r = chunk_server->ReleaseChunk(principal_id, vchunkdsc);
	return 0;
}


int
srv::name_lookup(const unsigned int principal_id, const std::string name, unsigned long long &r)
{
	dbg_log (DBG_CRITICAL, "Deprecated functionality\n");
/*
	int      ret;
	void*    obj;
	ret = name_service->Lookup(name.c_str(), &obj);
	if (ret<0) {
		return -ret;
	}
	r = (unsigned long long) obj;
	return 0;
*/
}


int
srv::name_link(const unsigned int principal_id, const std::string name, unsigned long long inode, int &r)
{
	dbg_log (DBG_CRITICAL, "Deprecated functionality\n");
/*
	int ret;

	ret = name_service->Link(name.c_str(), (void*) inode);
	if (ret<0) {
		return -ret;
	}
	return 0;
*/
}


int
srv::name_unlink(const unsigned int principal_id, const std::string name, int &r)
{
	dbg_log (DBG_CRITICAL, "Deprecated functionality\n");
/*
	int ret;

	ret = name_service->Unlink(name.c_str());
	if (ret<0) {
		return -ret;
	}
	return 0;
*/
}


int
srv::registry_lookup(const unsigned int principal_id, const std::string name, unsigned long long &r)
{
	int       ret;
	uint64_t  obj;
	ret = registry->Lookup(name.c_str(), &obj);
	if (ret<0) {
		return -ret;
	}
	r = (unsigned long long) obj;
	return 0;
}


int
srv::registry_add(const unsigned int principal_id, const std::string name, unsigned long long obj, int &r)
{
	int ret;

	ret = registry->Add(name, obj);
	if (ret<0) {
		return -ret;
	}
	return 0;
}


int
srv::registry_remove(const unsigned int principal_id, const std::string name, int &r)
{
	int ret;

	ret = registry->Remove(name);
	if (ret<0) {
		return -ret;
	}
	return 0;
}


int
srv::namespace_mount(const unsigned int principal_id, const std::string name, unsigned long long superblock, int &r)
{
	int ret;

	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");

	/*
	ret = name_space->Mount(name.c_str(), (void*) superblock);
	if (ret<0) {
		return -ret;
	}
	return 0;
	*/
}




srv service;

void startserver()
{
	serverp = new rpcs(port);
	serverp->reg(RPC_SERVER_IS_ALIVE, &service, &srv::alive);
	serverp->reg(RPC_CHUNK_CREATE, &service, &srv::chunk_create);
	serverp->reg(RPC_CHUNK_DELETE, &service, &srv::chunk_delete);
	serverp->reg(RPC_CHUNK_ACCESS, &service, &srv::chunk_access);
	serverp->reg(RPC_CHUNK_RELEASE, &service, &srv::chunk_release);
	serverp->reg(RPC_NAME_LOOKUP, &service, &srv::name_lookup);
	serverp->reg(RPC_NAME_LINK, &service, &srv::name_link);
	serverp->reg(RPC_NAME_UNLINK, &service, &srv::name_unlink);
	serverp->reg(RPC_REGISTRY_LOOKUP, &service, &srv::registry_lookup);
	serverp->reg(RPC_REGISTRY_ADD, &service, &srv::registry_add);
	serverp->reg(RPC_REGISTRY_REMOVE, &service, &srv::registry_remove);
	serverp->reg(RPC_NAMESPACE_MOUNT, &service, &srv::namespace_mount);

	serverp->reg(lock_protocol::stat, lm, &LockManager::Stat);
	serverp->reg(lock_protocol::acquire, lm, &LockManager::Acquire);
	serverp->reg(lock_protocol::acquirev, lm, &LockManager::AcquireVector);
	serverp->reg(lock_protocol::release, lm, &LockManager::Release);
	serverp->reg(lock_protocol::convert, lm, &LockManager::Convert);
	serverp->reg(lock_protocol::subscribe, lm, &LockManager::Subscribe);
}

int
main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	int debug_level = 0;

	srandom(getpid());
	port = 20000 + (getpid() % 10000);

	char ch = 0;
	while ((ch = getopt(argc, argv, "csd:p:lT:"))!=-1) {
		switch (ch) {
			case 'T':
				/* test framework argument -- ignore */
				break;
			case 'd':
				debug_level = atoi(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'l':
				assert(setenv("RPC_LOSSY", "5", 1) == 0);
			default:
				break;
		}
	}

	dbg_init(debug_level, NULL);

	pthread_attr_init(&attr);
	// set stack size to 32K, so we don't run out of memory
	pthread_attr_setstacksize(&attr, 32*1024);

	printf("Starting file system server on port %d RPC_HEADER_SZ %d\n", port, RPC_HEADER_SZ);

	chunk_server = new ChunkServer();
	chunk_server->Init();
	registry = new RegistryServer();
	registry->Init();
	lm = new LockManager();

	startserver();

	while (1) {
		sleep(1);
	}
}
