#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>
#include <iostream>
#include <vector>
#include "ipc/backend/rpc.h"
#include "chunkstore/chunkserver.h"
#include "common/debug.h"
#include "dpo/main/server/dpo.h"
#include "api.h"


extern ChunkServer*        chunk_server;
extern dpo::server::Dpo*   dpo_layer;

// server-side handlers. they must be methods of some class
// to simplify rpcs::reg(). a server process can have handlers
// from multiple classes.
class srv {
	public:
		int chunk_create(const unsigned int principal_id, const unsigned long long size, const int type, unsigned long long& r);
		int chunk_delete(const unsigned int principal_id, unsigned long long chunkdsc, int & r);
		int chunk_access(const unsigned int principal_id, std::vector<unsigned long long> vuchunkdsc, unsigned int, int& r);
		int chunk_release(const unsigned int principal_id, std::vector<unsigned long long> vuchunkdsc, int& r);
        int name_lookup(const unsigned int principal_id, const std::string name, unsigned long long& r);
        int name_link(const unsigned int principal_id, const std::string name, unsigned long long inode, int& r);
		int name_unlink(const unsigned int principal_id, const std::string name, int &r);
};

srv service;


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


void register_handlers(rpcs* serverp)
{
	serverp->reg(RPC_CHUNK_CREATE, &service, &srv::chunk_create);
	serverp->reg(RPC_CHUNK_DELETE, &service, &srv::chunk_delete);
	serverp->reg(RPC_CHUNK_ACCESS, &service, &srv::chunk_access);
	serverp->reg(RPC_CHUNK_RELEASE, &service, &srv::chunk_release);
	serverp->reg(RPC_NAME_LOOKUP, &service, &srv::name_lookup);
	serverp->reg(RPC_NAME_LINK, &service, &srv::name_link);
	serverp->reg(RPC_NAME_UNLINK, &service, &srv::name_unlink);
}
