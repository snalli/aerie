#define  __CACHE_GUARD__

#include "pxfs/client/client_i.h"
#include <sys/resource.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <iostream>
#include "bcs/bcs.h"
#include "pxfs/client/file.h"
#include "pxfs/client/fsomgr.h"
#include "osd/main/client/osd.h"
#include "osd/main/client/stm.h"
#include "osd/main/client/salloc.h"
#include "osd/main/client/omgr.h"
#include "scm/pool/pool.h"
#include "pxfs/common/fs_protocol.h"
#include "pxfs/mfs/client/mfs.h"
#include "pxfs/common/publisher.h"
#include "common/prof.h"
#include <libgen.h>
#include <stdio.h>
//#include "pxfs/client/cache.h"
//#define PROFILER_SAMPLE __PROFILER_SAMPLE

// FIXME: Client should be a singleton. otherwise we lose control 
// over instantiation and destruction of the global variables below, which
// should really be members of client
namespace client {

__thread Session* thread_session;

FileSystemObjectManager*    global_fsomgr;
FileManager*                global_fmgr;
NameSpace*                  global_namespace;
Ipc*                        global_ipc_layer;
osd::client::StorageSystem* global_storage_system;

#ifdef CONFIG_CACHE

//osd::common::Cache	    cache;
stats createNewFile=0, createNewDir=0;
stats createExistingFile=0, createExistingDir=0;
stats openExistingFile=0;
stats statFile=0;
stats delFile=0, delDir=0;
#endif

int 
Client::Init(const char* xdst, int debug_level) 
{

	////printf("\n Sanketh : Inside Client::Init...\n");	
        s_log("[%ld] %s",s_tid,__func__);

	int            ret;
	struct rlimit  rlim_nofile;

	if ((ret = Config::Init()) < 0) {
		//printf("\n Sanketh : Client::Init failure 1...\n");
		return ret;
	}
	if ((ret = Debug::Init(debug_level, NULL)) < 0) {
		//printf("\n Sanketh : Client::Init failure 2...\n");
		return ret;
	}

	global_ipc_layer = new Ipc(xdst);
	global_ipc_layer->Init();


	global_storage_system = new osd::client::StorageSystem(global_ipc_layer);
	global_storage_system->Init();
	// file manager should allocate file descriptors outside OS's range
	// to avoid collisions
	getrlimit(RLIMIT_NOFILE, &rlim_nofile);
	global_fmgr = new FileManager(rlim_nofile.rlim_max, 
	                              rlim_nofile.rlim_max+client::limits::kFileN);
	if ((ret = global_fmgr->Init()) < 0) {
		//printf("\n Sanketh : Client::Init failure 3...\n");
		return ret;	
	}

	// register statically known file system backends
	global_fsomgr = new FileSystemObjectManager();
	mfs::client::RegisterBackend(global_fsomgr);

	global_namespace = new NameSpace("GLOBAL");
	ret =  global_namespace->Init(CurrentSession());
	#ifdef CONFIG_CACHE
 //       printf("\n%d Cache : %016llX from %s\n",getpid(),&cache, __func__);

//	global_namespace->SetCache(&cache);
	#endif
	//printf("\n Sanketh : Return from NameSpace::Init : %d \n", ret);
	return ret;
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

	return Init(xdst, debug_level);
}


int 
Client::Shutdown() 
{
	        s_log("[%ld] %s",s_tid,__func__);

	// TODO: properly destroy any state created
	//printf("\n Sanketh : Shutting down libfs...\n");
	#ifdef CONFIG_CACHE
//	cache.cache_stats();
/*		//printf("\nTotal files created : %d", createNewFile);
		//printf("\nTotal dirs  created : %d", createNewDir);
		//printf("\nTotal open  ops     : %d + %d", openExistingFile, createExistingFile);
		//printf("\nTotal stat  ops     : %d", statFile);
		float hit_rate, cache_hit, cache_lookup;
		cache_hit = global_namespace->cacheHit;
		cache_lookup = global_namespace->cacheLookup;
		hit_rate = 100 * (cache_hit / cache_lookup);
		printf("\n***************************************************\n");
		printf("\nPID : %d",getpid());
 		printf("\nTerminating NameSpace...");
		printf("\nCache Hits : %d Hit rate : %0.2f %%", global_namespace->cacheHit, hit_rate);
		printf("\nCache Miss : %d", global_namespace->cacheMiss);
		printf("\nCache Accs : %d\n", global_namespace->cacheLookup);
		printf("Cache Inst : %d\n", global_namespace->succEntry);
		printf("\n***************************************************\n\n\n\n\n");
		sleep(2);
*/
	#endif
//	delete global_storage_system;

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
Client::Mount(const char* source, 
              const char* target, 
              const char* fstype, 
              uint32_t flags)
{
	
//	printf ("\nMounting...");
        s_log("[%ld] %s",s_tid,__func__);

	int                            ret;
	client::SuperBlock*            sb;
	char*                          path = const_cast<char*>(target);
	FileSystemProtocol::MountReply mntrep;

	dbg_log (DBG_INFO, "Mount file system %s of type %s to %s\n", source, fstype, target);
	
	if (target == NULL || source == NULL || fstype == NULL) {
		//printf("\n Sanketh : Client::Mount failure 1...\n");
		return -E_INVAL;
	}

	if ((ret = global_ipc_layer->call(FileSystemProtocol::kMount, 
	                                  global_ipc_layer->id(), std::string(source), 
	                                  std::string(target), flags, mntrep)) < 0) 
	{
		//printf("\n Sanketh : Client::Mount failure 2...\n");
		return -E_IPC;
	}
	if (ret > 0) {
		//printf("\n Sanketh : Client::Mount failure 3...\n");
		return -ret;
	}
	if ((ret = global_storage_system->Mount(source, target, flags, mntrep.desc_)) < 0) {
		//printf("\n Sanketh : Client::Mount failure 4...\n");
		return ret;
	}
	if ((ret = global_fsomgr->LoadSuperBlock(CurrentSession(), mntrep.desc_.oid_, fstype, &sb)) < 0) {
		//printf("\n Sanketh : Client::Mount failure 5...\n");
		return ret;
	}
	ret = global_namespace->Mount(CurrentSession(), path, sb);
//	printf("SUCCESS",ret);
	return ret;
}

void *
Client::lock_cont(){

	char name[128];
	Inode *dp;
	int ret;
	if ((ret = global_namespace->Nameiparent(CurrentSession(), "/pxfs/b/dd/ee/ff/file.txt",\
                                                 lock_protocol::Mode::IS,\
                                                 name, &dp)) < 0) {
                return (void *)dp;
        }

	return 0x0;
}
// TODO: This must be parameterized by file system because it has to 
// allocate an inode specific to the file system
// One way would be to keep a pointer to the superblock in the inode 
// TODO: Support O_EXCL with O_CREAT
//
// returns with the inode ipp referenced (get) and locked
static inline int
create(::client::Session* session, const char* path, Inode** ipp, int mode, int type)
{

	//printf("\nInside create...");
        s_log("[%ld] %s %s ",s_tid, __func__, path);
	PROFILER_PREAMBLE
	char          name[128];
	Inode*        dp; // Directory pointer ? (to pwd)
	Inode*        ip; // Inode pointer
	int           ret;

	dbg_log (DBG_INFO, "Create %s\n", path);

	PROFILER_SAMPLE

	#ifdef CONFIG_CACHE
		char dup_name[128];
		strcpy(dup_name, path);
	#endif

	// when Nameiparent returns successfully, dp is 
	// locked for writing. we release the lock on dp after we get the lock
	// on its child
	if ((ret = global_namespace->Nameiparent(session, path, 
	                                         lock_protocol::Mode::XL, 
	                                         name, &dp)) < 0) {
		return ret;
	}
//	if (!strcmp(path, "/txfs/a/b")) { return 0; }
	PROFILER_SAMPLE

	if ((ret = dp->Lookup(session, name, 0, &ip)) == E_SUCCESS) { // insight : If true, it means the file exists.
		// FIXME: if we create a file, do we need XR?
		#ifdef CONFIG_CACHE
			global_namespace->Insert(dup_name, ip, dp); 
		#endif

		ip->Lock(session, dp, lock_protocol::Mode::XR); 
		if (type == kFileInode && 
		    ip->type() == kFileInode) 
		{
//			++createExistingFile;
			*ipp = ip;
			dp->Put();
			dp->Unlock(session);
			return E_SUCCESS;
		}
//		++createExistingDir;
		ip->Put();
		ip->Unlock(session);
		dp->Put();
		dp->Unlock(session);
		return -E_EXIST;
	}
	PROFILER_SAMPLE

	session->journal()->TransactionBegin();
	// insight : If you have reached here, it means the file does not exist. We gotta create it.	
	PROFILER_SAMPLE
	// allocated inode is write locked and referenced (refcnt=1)
	if ((ret = global_fsomgr->CreateInode(session, dp, type, &ip)) < 0) {    ////////////// START HERE : In the path "/a/b/c", we have dp = b's inode, ip is still empty 
		//TODO: handle error; release directory inode
		assert(0 && "PANIC");
	}
	#ifdef CONFIG_CACHE
	#ifdef CONFIG_CALLBACK
		ip->Christen(dup_name, &cache);
		
	#endif
	#endif
	PROFILER_SAMPLE
	switch (type) {
		case kFileInode:
			session->journal() << Publisher::Message::LogicalOperation::MakeFile(dp->ino(), name, ip->ino());
		//	++createNewFile;
			break;
		case kDirInode:
			session->journal() << Publisher::Message::LogicalOperation::MakeDir(dp->ino(), name, ip->ino());
		//	++createNewDir;
			break;
	}

	PROFILER_SAMPLE
	ip->set_nlink(1);
	if (type == kDirInode) {
		assert(dp->set_nlink(dp->nlink() + 1) == 0); // for child's ..
		assert(ip->Link(session, ".", ip, false) == 0 );
		assert(ip->Link(session, "..", dp, false) == 0);
	}
	assert(dp->Link(session, name, ip, false) == 0); //insight : <name,oid> are inserted here !
	PROFILER_SAMPLE
	session->journal()->TransactionCommit();
	// DONE : Insert in dentry here !
	#ifdef CONFIG_CACHE
		global_namespace->Insert(dup_name, ip, dp); 
	#endif
	// insight : This is where we insert into the dentry cache
	dp->Put();
	dp->Unlock(session);
	*ipp = ip;
	PROFILER_SAMPLE
	return 0;
}


int 
Client::Open(const char* path, int flags, int mode)
{

//	printf("\nIn Client::Open...");
        s_log("[%ld] %s %s",s_tid, __func__, path);

	PROFILER_PREAMBLE
	Inode*   ip;
	int      ret;
	int      fd;
	File*    fp;
	Session* session = CurrentSession();

/***********************************/
#ifdef CONFIG_CACHE
 Inode*   parent;
 Inode*   grandparent;
 char dirc[128],*dname, *dup_path;
 strcpy(dirc,path);
 dname = dirname(dirc);
 dup_path = dirc;
 strcpy(dup_path,path);
#endif
/***********************************/
	
	dbg_log (DBG_INFO, "Open file: path = %s ..., flags = 0x%x (%s%s%s)\n", path, flags,
	         flags & O_APPEND ? "A": "",
	         flags & O_CREAT  ? "C": "",
	         flags & O_RDONLY  ? "R": "");

	if ((ret = global_fmgr->AllocFile(&fp)) < 0) {
		// We allocate a file pointer
		return ret;
	}
	if ((fd = global_fmgr->AllocFd(fp)) < 0) {
		// We allocate a file descriptor
		global_fmgr->ReleaseFile(fp);
		// insight : fix memory leak here. release all objects so far acquired.	
		return fd;
	}
	
	if (flags & O_CREAT) {
		// returns with the inode ip referenced and locked
		if ((ret = create(session, path, &ip, mode, kFileInode)) < 0) {
			return ret;
		}	
		//FIXME: we need to tell the lock manager to keep a capability around 
		//as we might need to acquire the lock again after someone deleted or moved
		//the file
	} else {
		#ifdef CLEAN_LKUP
		if (flags & O_RDWR || flags & O_WRONLY){
			global_namespace->Nameiparent(session, path, lock_mode, &ip);
		} else {
		}
		#endif
		lock_protocol::Mode lock_mode;
		if (flags & O_RDWR || flags & O_WRONLY) {
			lock_mode = lock_protocol::Mode::XL; // FIXME: do we need XL, or SL is good enough?
		} else {
			lock_mode = lock_protocol::Mode::SL; // FIXME: do we need XL, or SL is good enough?
			//lock_mode = lock_protocol::Mode::IS; // FIXME: do we need XL, or SL is good enough?
		}
	//	lock_mode = lock_protocol::Mode::XL; // FIXME: do we need XL, or SL is good enough?
		PROFILER_SAMPLE
		#ifdef LCK_MGR
		if ((ret = global_namespace->Namei(session, path, lock_mode, &ip)) < 0) {
			return ret;
		}	
		#else
		ret = global_namespace->namei_sans_locks(session, path, lock_mode, &ip);
		if(ret < 0)
			return ret;
		fp->Init(ip, flags);
		return fd;
		#endif

		PROFILER_SAMPLE
	}
	fp->Init(ip, flags);
	ip->Unlock(session);

	dbg_log (DBG_INFO, "Open file: path = %s, fd=%d, ino=%p: DONE\n", path, fd, (void*) ip->ino());

	return fd;
}

void
Client::Prefetch()
{
	#ifdef CONFIG_CACHE
	global_namespace->Prefetch(CurrentSession());
	#endif
}

int
Client::Close(int fd)
{

	//printf("\n Sanketh : Closing file... \n");
        s_log("[%ld] Client::%s %d",s_tid, __func__, fd);

	dbg_log (DBG_INFO, "Close file: fd=%d\n", fd);

	int ret = global_fmgr->Put(fd);


	return ret;
}


int
Client::Duplicate(int oldfd)
{
	File* fp;
	return global_fmgr->Get(oldfd, &fp);
}


int
Client::Duplicate(int oldfd, int newfd)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
	return E_SUCCESS;
}


int 
Client::WriteOffset(int fd, const char* src, uint64_t n, uint64_t offset)
{
	File* fp;
	int   ret;

	dbg_log (DBG_INFO, "Write file: fd=%d\n", fd);
        s_log("[%ld] %s %d",s_tid, __func__, fd);


	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Write(CurrentSession(), src, n, offset);
}


int 
Client::ReadOffset(int fd, char* dst, uint64_t n, uint64_t offset)
{
	int   ret;
	File* fp;

        s_log("[%ld] %s %d",s_tid, __func__, fd);

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Read(CurrentSession(), dst, n, offset);
}


int 
Client::Write(int fd, const char* src, uint64_t n)
{

	//printf("\n Sanketh : Writing to file ...\n");
	File* fp;
	int   ret;
	
	dbg_log (DBG_INFO, "Write file: fd=%d, n=%lu\n", fd, n);

        s_log("[%ld] Client::%s %d",s_tid, __func__, fd);

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		//printf("\n Sanketh : Client::Write failure 1 \n");
		return ret;
	}
	return fp->Write(CurrentSession(), src, n);
}


int 
Client::Read(int fd, char* dst, uint64_t n)
{

	//printf("\n Sanketh : Reading file...\n");

	int   ret;
	File* fp;

	dbg_log (DBG_INFO, "Read file: fd=%d, n=%lu\n", fd, n);

        s_log("[%ld] %s %d",s_tid, __func__, fd);

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Read(CurrentSession(), dst, n);
}



uint64_t 
Client::Seek(int fd, uint64_t offset, int whence)
{
	int   ret;
	File* fp;

        s_log("[%ld] %s %d",s_tid, __func__, fd);

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Seek(CurrentSession(), offset, whence);
}


int
Client::CreateDir(const char* path, int mode)
{

	//printf("\n Sanketh : Inside Create. Creating dir %s...\n", path);	
        s_log("[%ld] %s %s",s_tid, __func__, path);

	int      ret;
	Inode*   ip;
	Session* session = CurrentSession();

	dbg_log (DBG_INFO, "Create Directory: %s\n", path);	

	if ((ret = create(session, path, &ip, mode, kDirInode)) < 0) {
		return ret;
	}
	assert(ip->Put() == E_SUCCESS);
	assert(ip->Unlock(session) == E_SUCCESS);
	return 0;
}


int
Client::DeleteDir(const char* pathname)
{

	//printf("\n Sanketh : Deleting directory %s...\n",pathname);
        s_log("[%ld] %s %s",s_tid,__func__,pathname);

	dbg_log (DBG_INFO, "Delete Directory: %s\n", pathname);	
	int ret;
	if((ret=Client::Unlink(pathname))) {
	//	++delDir;
		return ret;
	}
	return ret;
}


int
Client::SetCurWrkDir(const char* path)
{
	Session* session = CurrentSession();
        s_log("[%ld] %s %s",s_tid, __func__, path);

	return global_namespace->SetCurWrkDir(session, path);
}


int
Client::GetCurWrkDir(const char* path, size_t size)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
        s_log("[%ld] %s %s",s_tid, __func__, path);

	return E_SUCCESS;
}


int 
Client::Rename(const char* oldpath, const char* newpath)
{
	return global_namespace->Rename(CurrentSession(), oldpath, newpath);
}


int 
Client::Link(const char* oldpath, const char* newpath)
{
	return global_namespace->Link(CurrentSession(), oldpath, newpath);
}


int 
Client::Unlink(const char* pathname)
{
        s_log("[%ld] Client::%s %s",s_tid, __func__, pathname);

	dbg_log (DBG_INFO, "Unlink: %s\n", pathname);	

	return global_namespace->Unlink(CurrentSession(), pathname);
}



int 
Client::Stat(const char *path, struct stat *buf)
{
	int                 ret;
	lock_protocol::Mode lock_mode = lock_protocol::Mode::XL; 
	Session*            session = CurrentSession();
	Inode*              ip;

        s_log("[%ld] %s %s",s_tid, __func__, path);

	if ((ret = global_namespace->Namei(session, path, lock_mode, &ip)) < 0) {
		return ret;
	}
//	++statFile;
	ip->Put();
	ip->Unlock(session);

	return ret;
}


int 
Client::Sync()
{
	Session* session = CurrentSession();
	session->omgr()->CloseAllObjects(session, true);
	return E_SUCCESS;
}


// current pxfs synchronously writes data and metadata
int 
Client::Sync(int fd)
{
	return E_SUCCESS;
}


/// Check whether we can communicate with the server
int
Client::TestServerIsAlive()
{
	return global_ipc_layer->Test();
}


} // namespace client
