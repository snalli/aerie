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
#include "nameservice.h"
#include "api.h"

rpcs*          server;  // server rpc object
int            port;
pthread_attr_t attr;
NameService*   name_service;
ChunkServer*   chunk_server;


// server-side handlers. they must be methods of some class
// to simplify rpcs::reg(). a server process can have handlers
// from multiple classes.
class srv {
	public:
		int chunk_create(const unsigned int principal_id, const unsigned long long size, unsigned long long & r);
		int chunk_delete(const unsigned int principal_id, unsigned long long chunkdsc, int & r);
		int chunk_access(const unsigned int principal_id, std::vector<unsigned long long> vuchunkdsc, unsigned int, int & r);
		int chunk_release(const unsigned int principal_id, std::vector<unsigned long long> vuchunkdsc, int & r);
        int name_lookup(const unsigned int principal_id, const std::string name, unsigned long long &r);
        int name_link(const unsigned int principal_id, const std::string name, unsigned long long inode, int &r);
		int name_remove(const unsigned int principal_id, const std::string name, int &r);
};

int
srv::chunk_create(const unsigned int principal_id, const unsigned long long size, unsigned long long &r)
{
	ChunkDescriptor*   chunkdsc;
	unsigned long long chunkdsc_id;
	int                ret;

	ret = chunk_server->CreateChunk(principal_id, size, &chunkdsc);
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
	int      ret;
	inode_t* inode;
	ret = name_service->Lookup(name.c_str(), &inode);
	if (ret<0) {
		return -ret;
	}
	r = (unsigned long long) inode;
	return 0;
}


int
srv::name_link(const unsigned int principal_id, const std::string name, unsigned long long inode, int &r)
{
	int ret;

	ret = name_service->Link(name.c_str(), (void*) inode);
	if (ret<0) {
		return -ret;
	}
	return 0;
}


int
srv::name_remove(const unsigned int principal_id, const std::string name, int &r)
{
	int ret;

	ret = name_service->Remove(name.c_str());
	if (ret<0) {
		return -ret;
	}
	return 0;
}


srv service;

void startserver()
{
	server = new rpcs(port);
	server->reg(RPC_CHUNK_CREATE, &service, &srv::chunk_create);
	server->reg(RPC_CHUNK_DELETE, &service, &srv::chunk_delete);
	server->reg(RPC_CHUNK_ACCESS, &service, &srv::chunk_access);
	server->reg(RPC_CHUNK_RELEASE, &service, &srv::chunk_release);
	server->reg(RPC_NAME_LOOKUP, &service, &srv::name_lookup);
	server->reg(RPC_NAME_LINK, &service, &srv::name_link);
	server->reg(RPC_NAME_REMOVE, &service, &srv::name_remove);
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
	while ((ch = getopt(argc, argv, "csd:p:l"))!=-1) {
		switch (ch) {
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

	if (debug_level > 0) {
		jsl_set_debug(debug_level);
		jsl_log(JSL_DBG_1, "DEBUG LEVEL: %d\n", debug_level);
	}

	pthread_attr_init(&attr);
	// set stack size to 32K, so we don't run out of memory
	pthread_attr_setstacksize(&attr, 32*1024);

	printf("starting server on port %d RPC_HEADER_SZ %d\n", port, RPC_HEADER_SZ);

	name_service = new NameService();
	chunk_server = new ChunkServer();
	chunk_server->Init();

	startserver();

	while (1) {
		sleep(1);
	}
}
