#include "rxfs/client/client.h"
#include <sys/resource.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <iostream>
#include "bcs/bcs.h"
#include "rxfs/client/file.h"
#include "rxfs/client/sb.h"
#include "osd/main/client/osd.h"
#include "osd/main/client/stm.h"
#include "osd/main/client/salloc.h"
#include "osd/main/client/omgr.h"
#include "scm/pool/pool.h"
#include "pxfs/common/fs_protocol.h" // we use the pxfs server
#include "pxfs/common/publisher.h"   // we use the pxfs server
#include "common/prof.h"

//#define PROFILER_SAMPLE __PROFILER_SAMPLE

// FIXME: Client should be a singleton. otherwise we lose control 
// over instantiation and destruction of the global variables below, which
// should really be members of client
namespace rxfs {
namespace client {

__thread Session* thread_session;

FileManager*                global_fmgr;
NameSpace*                  global_namespace;
::client::Ipc*              global_ipc_layer;
osd::client::StorageSystem* global_storage_system;



int 
Client::Init(const char* xdst, int debug_level) 
{
	int            ret;
	struct rlimit  rlim_nofile;

	if ((ret = Config::Init()) < 0) {
		return ret;
	}
	if ((ret = Debug::Init(debug_level, NULL)) < 0) {
		return ret;
	}

	global_ipc_layer = new ::client::Ipc(xdst);
	global_ipc_layer->Init();


	global_storage_system = new osd::client::StorageSystem(global_ipc_layer);
	global_storage_system->Init();
	// file manager should allocate file descriptors outside OS's range
	// to avoid collisions
	getrlimit(RLIMIT_NOFILE, &rlim_nofile);
	global_fmgr = new FileManager(rlim_nofile.rlim_max, 
	                              rlim_nofile.rlim_max+ ::rxfs::client::limits::kFileN);
	if ((ret = global_fmgr->Init()) < 0) {
		return ret;	
	}

	global_namespace = NULL;
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

	return Init(xdst, debug_level);
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
Client::Mount(const char* source, 
              const char* target, 
              const char* fstype, 
              uint32_t flags)
{
	InodeNumber                    root_ino;
	int                            ret;
	char*                          path = const_cast<char*>(target);
	FileSystemProtocol::MountReply mntrep;
	Session*                       session = CurrentSession();
	SuperBlock                     sb;

	dbg_log (DBG_INFO, "Mount file system %s of type %s to %s\n", source, fstype, target);
	
	if (target == NULL || source == NULL || fstype == NULL) {
		return -E_INVAL;
	}

	if ((ret = global_ipc_layer->call(FileSystemProtocol::kMount, 
	                                  global_ipc_layer->id(), std::string(source), 
	                                  std::string(target), flags, mntrep)) < 0) 
	{
		return -E_IPC;
	}
	if (ret > 0) {
		return -ret;
	}
	if ((ret = global_storage_system->Mount(source, target, flags, mntrep.desc_)) < 0) {
		return ret;
	}
	SuperBlock::Load(session, mntrep.desc_.oid_, &sb);
	global_namespace = new NameSpace(target, sb.root_ino());
	return E_SUCCESS;
}


int 
Client::Open(const char* path, int flags, int mode, File** file)
{
	PROFILER_PREAMBLE
	Inode*      ip;
	int         ret;
	int         fd;
	File*       fp;
	Session*    session = CurrentSession();
	InodeNumber ino;
	PROFILER_SAMPLE
	
	if ((ret = global_fmgr->AllocFile(session, &fp)) < 0) {
		return ret;
	}

	
	PROFILER_SAMPLE
	if (flags & O_CREAT) {
		dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
	} else {
		lock_protocol::Mode lock_mode = lock_protocol::Mode::XL; 
		if ((ret = global_namespace->Namei(session, path, lock_mode, &ino)) < 0) {
			return ret;
		}	
	}
	PROFILER_SAMPLE
	fp->Init(ino, flags);
	PROFILER_SAMPLE

	*file = fp;
	return E_SUCCESS;
}


int 
Client::Open(const char* path, int flags, int mode)
{
	PROFILER_PREAMBLE
	Inode*      ip;
	int         ret;
	int         fd;
	File*       fp;
	Session*    session = CurrentSession();
	PROFILER_SAMPLE
	
	if ((ret = Open(path, flags, mode, &fp)) < 0) {
		return ret;
	}
	if ((fd = global_fmgr->AllocFd(fp)) < 0) {
		global_fmgr->ReleaseFile(session, fp);
		return fd;
	}
	return fd;
}


int
Client::Close(int fd)
{
	Session* session = CurrentSession();
	return global_fmgr->Put(session, fd);
}


int
Client::Close(File* file)
{
	Session* session = CurrentSession();
	return global_fmgr->ReleaseFile(session, file);
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
Client::WriteOffset(File* fp, const char* src, uint64_t n, uint64_t offset)
{
	return fp->Write(CurrentSession(), src, n, offset);
}


int 
Client::WriteOffset(int fd, const char* src, uint64_t n, uint64_t offset)
{
	File* fp;
	int   ret;

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Write(CurrentSession(), src, n, offset);
}


int 
Client::ReadOffset(File* fp, char* dst, uint64_t n, uint64_t offset)
{
	return fp->Read(CurrentSession(), dst, n, offset);
}


int 
Client::ReadOffset(int fd, char* dst, uint64_t n, uint64_t offset)
{
	int   ret;
	File* fp;

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Read(CurrentSession(), dst, n, offset);
}


int 
Client::Write(File* fp, const char* src, uint64_t n)
{
	return fp->Write(CurrentSession(), src, n);
}


int 
Client::Write(int fd, const char* src, uint64_t n)
{
	File* fp;
	int   ret;

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Write(CurrentSession(), src, n);
}


int 
Client::Read(File* fp, char* dst, uint64_t n)
{
	return fp->Read(CurrentSession(), dst, n);
}


int 
Client::Read(int fd, char* dst, uint64_t n)
{
	int   ret;
	File* fp;

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

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Seek(CurrentSession(), offset, whence);
}


int
Client::CreateDir(const char* path, int mode)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
	return E_SUCCESS;
}


int
Client::DeleteDir(const char* pathname)
{
	dbg_log (DBG_INFO, "Delete Directory: %s\n", pathname);	

	return Client::Unlink(pathname);
}


int
Client::SetCurWrkDir(const char* path)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
	return E_SUCCESS;
}


int
Client::GetCurWrkDir(const char* path, size_t size)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
	return E_SUCCESS;
}


int 
Client::Rename(const char* oldpath, const char* newpath)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
	return E_SUCCESS;
}


int 
Client::Link(const char* oldpath, const char* newpath)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
	return E_SUCCESS;
}


int 
Client::Unlink(const char* pathname)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
	return E_SUCCESS;
}



int 
Client::Stat(const char *path, struct stat *buf)
{
	int                 ret;
	lock_protocol::Mode lock_mode = lock_protocol::Mode::XL; 
	Session*            session = CurrentSession();
	InodeNumber         ino;

	if ((ret = global_namespace->Namei(session, path, lock_mode, &ino)) < 0) {
		return ret;
	}

	return ret;
}


int 
Client::Sync()
{
	Session* session = CurrentSession();
	session->omgr()->CloseAllObjects(session, true);
	return E_SUCCESS;
}


// current rxfs synchronously writes data and metadata
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
} // namespace rxfs 
