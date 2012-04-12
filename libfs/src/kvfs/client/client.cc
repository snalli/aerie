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
#include "spa/pool/pool.h"
#include "kvfs/client/session.h"
#include "kvfs/common/fs_protocol.h"
#include "kvfs/common/publisher.h"
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



int 
Client::Init(const char* xdst) 
{
	int     ret;

	Config::Init();
	
	global_ipc_layer = new Ipc(xdst);
	global_ipc_layer->Init();


	global_storage_system = new osd::client::StorageSystem(global_ipc_layer);
	global_storage_system->Init();

	// register statically known file system backends
	//global_fsomgr = new FileSystemObjectManager();
	//mfs::client::RegisterBackend(global_fsomgr);

	//global_namespace = new NameSpace("GLOBAL");
	//return global_namespace->Init(CurrentSession());
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
	//client::SuperBlock*            sb;
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
	//if ((ret = global_fsomgr->LoadSuperBlock(CurrentSession(), mntrep.desc_.oid_, fstype, &sb)) < 0) {
	//	return ret;
	//}
	//return global_namespace->Mount(CurrentSession(), path, sb);
	return E_SUCCESS;
}


#if 0

// returns with the inode ipp referenced (get) and locked
static inline int
create(::client::Session* session, const char* path, Inode** ipp, int mode, int type)
{
	PROFILER_PREAMBLE
	char          name[128];
	Inode*        dp;
	Inode*        ip;
	int           ret;

	dbg_log (DBG_INFO, "Create %s\n", path);

	PROFILER_SAMPLE

	// when Nameiparent returns successfully, dp is 
	// locked for writing. we release the lock on dp after we get the lock
	// on its child
	if ((ret = global_namespace->Nameiparent(session, path, 
	                                         lock_protocol::Mode::XL, 
	                                         name, &dp)) < 0) {
		return ret;
	}
	PROFILER_SAMPLE

	if ((ret = dp->Lookup(session, name, 0, &ip)) == E_SUCCESS) {
		// FIXME: if we create a file, do we need XR?
		ip->Lock(session, dp, lock_protocol::Mode::XR); 
		if (type == kFileInode && 
		    ip->type() == kFileInode) 
		{
			*ipp = ip;
			dp->Put();
			dp->Unlock(session);
			return E_SUCCESS;
		}
		ip->Put();
		ip->Unlock(session);
		dp->Put();
		dp->Unlock(session);
		return -E_EXIST;
	}
	PROFILER_SAMPLE

	session->journal()->TransactionBegin();
	
	PROFILER_SAMPLE
	// allocated inode is write locked and referenced (refcnt=1)
	if ((ret = global_fsomgr->CreateInode(session, dp, type, &ip)) < 0) {
		//TODO: handle error; release directory inode
		assert(0 && "PANIC");
	}
	PROFILER_SAMPLE
	switch (type) {
		case kFileInode:
			session->journal() << Publisher::Message::LogicalOperation::MakeFile(dp->ino(), name, ip->ino());
			break;
		case kDirInode:
			session->journal() << Publisher::Message::LogicalOperation::MakeDir(dp->ino(), name, ip->ino());
			break;
	}

	PROFILER_SAMPLE
	ip->set_nlink(1);
	if (type == kDirInode) {
		assert(dp->set_nlink(dp->nlink() + 1) == 0); // for child's ..
		assert(ip->Link(session, ".", ip, false) == 0 );
		assert(ip->Link(session, "..", dp, false) == 0);
	}
	assert(dp->Link(session, name, ip, false) == 0);
	PROFILER_SAMPLE
	session->journal()->TransactionCommit();
	dp->Put();
	dp->Unlock(session);
	*ipp = ip;
	PROFILER_SAMPLE
	return 0;
}


int 
Client::Open(const char* path, int flags, int mode)
{
	PROFILER_PREAMBLE
	Inode*   ip;
	int      ret;
	int      fd;
	File*    fp;
	Session* session = CurrentSession();
	
	PROFILER_SAMPLE
	if ((ret = global_fmgr->AllocFile(&fp)) < 0) {
		return ret;
	}
	if ((fd = global_fmgr->AllocFd(fp)) < 0) {
		global_fmgr->ReleaseFile(fp);
		return fd;
	}
	
	PROFILER_SAMPLE
	if (flags & O_CREAT) {
		// returns with the inode ip referenced and locked
		if ((ret = create(session, path, &ip, mode, kFileInode)) < 0) {
			return ret;
		}	
		//FIXME: we need to tell the lock manager to keep a capability around 
		//as we might need to acquire the lock again after someone deleted or moved
		//the file
	} else {
		lock_protocol::Mode lock_mode = lock_protocol::Mode::XL; // FIXME: do we need XL, or SL is good enough?
		if ((ret = global_namespace->Namei(session, path, lock_mode, &ip)) < 0) {
			return ret;
		}	
		PROFILER_SAMPLE
	}
	fp->Init(ip, flags);
	ip->Unlock(session);

	return fd;
}


#endif

int 
Client::Put(const char* key, const char* src, uint64_t n)
{
	int   ret;

	//return fp->Write(CurrentSession(), src, n);
	return 0;
}


int 
Client::Get(const char* key, char* dst, uint64_t n)
{
	int   ret;

	//return fp->Read(CurrentSession(), dst, n);
	return 0;
}


int 
Client::Delete(const char* key)
{
	//return global_namespace->Unlink(CurrentSession(), pathname);
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
