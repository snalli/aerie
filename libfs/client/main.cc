#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <iostream>
#include "rpc/rpc.h"
#include "rpc/jsl_log.h"
#include "chunkstore/chunkstore.h"
#include "nameservice.h"


rpcc* client;  // client rpc object
ChunkStore* chunk_store;
struct sockaddr_in dst; //server's ip address
pthread_attr_t attr;
unsigned int principal_id;
NameService* name_service;

void simple_tests()
{
	ChunkDescriptor* chunkdsc;
	chunk_store->CreateChunk(1024, &chunkdsc);
	chunk_store->CreateChunk(1024, &chunkdsc);
}

void simple_names()
{
	int      ret;
	inode_t* inode;

	name_service->Link("/test/A", (inode_t*) 0xA);
	ret = name_service->Link("/test/B", (inode_t*) 0xB);
	printf("delete: %d\n", name_service->Remove("/test/B"));

}


int
main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
    int port;
	int debug_level = 0;
	uid_t principal_id;

	principal_id = getuid();
	srandom(getpid());
	port = 20000 + (getpid() % 10000);

	char ch = 0;
	while ((ch = getopt(argc, argv, "csd:p:li:"))!=-1) {
		switch (ch) {
			case 'd':
				debug_level = atoi(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'l':
				assert(setenv("RPC_LOSSY", "5", 1) == 0);
				break;
			case 'i':
				principal_id = atoi(optarg);
				break;
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
	
	chunk_store = new ChunkStore(principal_id);
	chunk_store->Init();
	//simple_tests();

	// server's address.
	memset(&dst, 0, sizeof(dst));
	dst.sin_family = AF_INET;
	dst.sin_addr.s_addr = inet_addr("127.0.0.1");
	dst.sin_port = htons(port);

	// start the client.  bind it to the server.
	// starts a thread to listen for replies and hand them to
	// the correct waiting caller thread. there should probably
	// be only one rpcc per process. you probably need one
	// rpcc per server.
	client = new rpcc(dst);
	assert (client->bind() == 0);

	name_service = new NameService(client, principal_id);

	simple_names();
}
