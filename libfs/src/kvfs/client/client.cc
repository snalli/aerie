#include "kvfs/client/client.h"
#include <sys/resource.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <iostream>
#include "bcs/bcs.h"
#include "osd/main/client/osd.h"
#include "osd/main/client/stm.h"
#include "osd/main/client/salloc.h"
#include "osd/main/client/omgr.h"
#include "scm/pool/pool.h"
#include "kvfs/client/session.h"
#include "kvfs/client/sb.h"
#include "kvfs/common/fs_protocol.h"
#include "kvfs/common/publisher.h"
#include "kvfs/client/table.h"
#include "common/prof.h"


// FIXME: Client should be a singleton. otherwise we lose control 
// over instantiation and destruction of the global variables below, which
// should really be members of client
namespace client {

__thread Session* thread_session;

//FileSystemObjectManager*    global_fsomgr;
//NameSpace*                  global_namespace;
Ipc*                        global_ipc_layer;
osd::client::StorageSystem* global_storage_system;

::client::Table*            Client::tp_;

int 
Client::Init(const char* xdst) 
{
	int     ret;

	dbg_log (DBG_INFO, "Initialize\n");

	Config::Init();
	
	global_ipc_layer = new Ipc(xdst);
	global_ipc_layer->Init();

	global_storage_system = new osd::client::StorageSystem(global_ipc_layer);
	global_storage_system->Init();

	return E_SUCCESS;
}


int 
Client::Init(int argc, char* argv[])
{
	int          ch;
	int          ret;
	int          debug_level = -1;
	const char*  xdst = NULL;

	while ((ch = getopt(argc, argv, "d:h:li:o:n:"))!=-1) {
		switch (ch) {
			case 'd':
				debug_level = atoi(optarg);
				break;
			case 'h':
				xdst = optarg;
				break;
			default:
				break;
		}
	}

	if ((ret = Config::Init()) < 0) {
		return ret;
	}
	if ((ret = Debug::Init(debug_level, NULL)) < 0) {
		return ret;
	}
	return Init(xdst);
}


int 
Client::Shutdown() 
{
	// TODO: properly destroy any state created
	delete global_storage_system;
	return 0;
}


Session*
Client::CurrentSession()
{
	if (thread_session) {
		return thread_session;
	}

	thread_session = new Session(global_storage_system);
	thread_session->tx_ = osd::stm::client::Self();
	return thread_session;
}


int 
Client::Mount(const char* source, uint32_t flags)
{
	int                            ret;
	client::SuperBlock*            sb;
	FileSystemProtocol::MountReply mntrep;

	dbg_log (DBG_INFO, "Mount key-value file system %s\n", source);
	
	if (source == NULL) {
		return -E_INVAL;
	}

	if ((ret = global_ipc_layer->call(FileSystemProtocol::kMount, 
	                                  global_ipc_layer->id(), std::string(source), 
	                                  flags, mntrep)) < 0) 
	{
		return -E_IPC;
	}
	if (ret > 0) {
		return -ret;
	}
	if ((ret = global_storage_system->Mount(source, flags, mntrep.desc_)) < 0) {
		return ret;
	}
	if ((ret = SuperBlock::Load(CurrentSession(), mntrep.desc_.oid_, &sb)) < 0) {
		return ret;
	}
	tp_ = sb->Table();
	return E_SUCCESS;
}


int 
Client::Put(const char* key, const char* src, uint64_t n)
{
	return tp_->Put(CurrentSession(), key, src, n);
}


int 
Client::Get(const char* key, char* dst)
{
	return tp_->Get(CurrentSession(), key, dst);
}


int 
Client::Delete(const char* key)
{
	return tp_->Erase(CurrentSession(), key);
}


int 
Client::Sync()
{
	Session* session = CurrentSession();
	session->omgr()->CloseAllObjects(session, true);
	return E_SUCCESS;
}


/// Check whether we can communicate with the server
int
Client::TestServerIsAlive()
{
	return global_ipc_layer->Test();
}


} // namespace client
