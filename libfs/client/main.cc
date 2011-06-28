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
#include "server/api.h"
#include "v6fs/v6fs.h"
#include "nameservice.h"


rpcc* client;  // client rpc object
ChunkStore* chunk_store;
struct sockaddr_in dst; //server's ip address
pthread_attr_t attr;
unsigned int principal_id;
NameService* global_nameservice;

void simple_tests()
{
	ChunkDescriptor* achunkdsc[16];
	void* a, *b, *c, *d;
	int tmp;

	ChunkDescriptor* chunkdsc;
	chunk_store->CreateChunk(16384, CHUNK_TYPE_EXTENT, &chunkdsc);
	achunkdsc[0] = chunkdsc;
	chunk_store->CreateChunk(8192, CHUNK_TYPE_INODE, &chunkdsc);
	achunkdsc[1] = chunkdsc;
	chunk_store->AccessChunkList(achunkdsc, 2, PROT_READ|PROT_WRITE);
	chunk_store->ReleaseChunkList(achunkdsc, 1);
}

void demo_name()
{
	int      ret;

	global_nameservice->Link("/test/A", (void*) 0xA);
	ret = global_nameservice->Link("/test/B", (void*) 0xB);
	printf("delete: %d\n", global_nameservice->Unlink("/test/B"));

}


int
main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
    int   port;
	int   debug_level = 0;
	uid_t principal_id;
	char  operation[16];
	char ch = 0;

	principal_id = getuid();
	srandom(getpid());
	port = 20000 + (getpid() % 10000);

	while ((ch = getopt(argc, argv, "d:p:li:o:"))!=-1) {
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
			case 'o':
				strcpy(operation, optarg); 
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
	global_nameservice = new NameService(client, principal_id, "GLOBAL");

	chunk_store = new ChunkStore(client, principal_id);
	chunk_store->Init();

	if (strcmp(operation, "mkfs") == 0) {
		mkfs();
	} else if (strcmp(operation, "demo") == 0) {
		demo_name();	
	} else {
		// unknown
	}

	return 0;
}
