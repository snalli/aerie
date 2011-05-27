#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>
#include <iostream>
#include "rpc/rpc.h"
#include "rpc/jsl_log.h"
#include "chunkstore/chunkserver.h"
#include "nameservice.h"

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
		int chunk_delete(const unsigned int principal_id, unsigned long long chunkdsc, const unsigned long long size, int & r);
		int chunk_access(const unsigned int principal_id, const unsigned long long chunkdsc, int & r);
        int lookup(const unsigned int principal_id, const std::string name, unsigned long long &r);
        int link(const unsigned int principal_id, const std::string name, unsigned long long inode, int &r);
		int remove(const unsigned int principal_id, const std::string name, int &r);
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
srv::chunk_delete(const unsigned int principal_id, const unsigned long long chunkdsc_id, const unsigned long long size, int &r)
{
	ChunkDescriptor* chunkdsc = (ChunkDescriptor*) chunkdsc_id;
	r = chunk_server->DeleteChunk(principal_id, chunkdsc);
	return 0;
}


int
srv::chunk_access(const unsigned int principal_id, const unsigned long long chunkdsc_id, int &r)
{
	ChunkDescriptor* chunkdsc = (ChunkDescriptor*) chunkdsc_id;
	r = chunk_server->AccessChunk(principal_id, chunkdsc);
	return 0;
}

int
srv::lookup(const unsigned int principal_id, const std::string name, unsigned long long &r)
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
srv::link(const unsigned int principal_id, const std::string name, unsigned long long inode, int &r)
{
	int ret;

	ret = name_service->Link(name.c_str(), (void*) inode);
	if (ret<0) {
		return -ret;
	}
	return 0;
}


int
srv::remove(const unsigned int principal_id, const std::string name, int &r)
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
	server->reg(22, &service, &srv::chunk_create);
	server->reg(23, &service, &srv::chunk_delete);
	server->reg(24, &service, &srv::chunk_access);
	server->reg(25, &service, &srv::lookup);
	server->reg(26, &service, &srv::link);
	server->reg(27, &service, &srv::remove);
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
