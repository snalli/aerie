#include "cfs/client/client_i.h"
#include <sys/resource.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <iostream>
#include "bcs/bcs.h"
#include "cfs/client/file.h"
#include "osd/main/client/osd.h"
#include "osd/main/client/stm.h"
#include "osd/main/client/salloc.h"
#include "osd/main/client/omgr.h"
#include "spa/pool/pool.h"
#include "cfs/common/fs_protocol.h"
#include "cfs/common/publisher.h"
#include "common/prof.h"
#include "cfs/common/types.h"
#include "cfs/common/shreg.h"


//#define PROFILER_SAMPLE __PROFILER_SAMPLE

//TODO: currently CFS does not support namespace resolutions relative to 
//current working directory

namespace client {

FileManager*                global_fmgr;
Ipc*                        global_ipc_layer;
osd::client::StorageSystem* global_storage_system;

SharedRegion* Client::shreg_;


int 
Client::Init(const char* xdst) 
{
	int            ret;
	struct rlimit  rlim_nofile;

	Config::Init();
	
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
		return ret;	
	}
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


int 
Client::Mount(const char* source, 
              const char* target, 
              const char* fstype, 
              uint32_t flags)
{
	int                            ret;
	char*                          path = const_cast<char*>(target);
	FileSystemProtocol::MountReply mntrep;

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
	shreg_ = new SharedRegion(global_ipc_layer->id());
	assert(shreg_->Open() == 0);
	return E_SUCCESS;
}


// TODO: This must be parameterized by file system because it has to 
// allocate an inode specific to the file system
// One way would be to keep a pointer to the superblock in the inode 
// TODO: Support O_EXCL with O_CREAT
//
// returns with the inode ipp referenced (get) and locked
static inline int
create(const char* path, int mode, int type, InodeNumber* inop)
{
	char                              name[128];
	int                               ret;
	FileSystemProtocol::InodeNumber   ino;

	dbg_log (DBG_INFO, "Create %s\n", path);

	switch (type) {
		case kDirInode:
			if ((ret = global_ipc_layer->call(FileSystemProtocol::kMakeDir, 
											  global_ipc_layer->id(), std::string(path), 
											  (unsigned int) mode, ino)) < 0) 
			{
				return -E_IPC;
			}
			if (ret > 0) {
				return -ret;
			}
			*inop = ino;
			ret = E_SUCCESS;
			break;
		case kFileInode:
			if ((ret = global_ipc_layer->call(FileSystemProtocol::kMakeFile, 
											  global_ipc_layer->id(), std::string(path), 
											  (unsigned int) 0, (unsigned int) mode, ino)) < 0) 
			{
				return -E_IPC;
			}
			if (ret > 0) {
				return -ret;
			}
			*inop = ino;
			ret = E_SUCCESS;
			break;
	}
	return ret;
}


int 
Client::Open(const char* path, int flags, int mode)
{
	int                             ret;
	int                             fd;
	File*                           fp;
	FileSystemProtocol::InodeNumber protocol_ino;
	InodeNumber                     ino;
	
	if ((ret = global_fmgr->AllocFile(&fp)) < 0) {
		return ret;
	}
	if ((fd = global_fmgr->AllocFd(fp)) < 0) {
		global_fmgr->ReleaseFile(fp);
		return fd;
	}
	
	if (flags & O_CREAT) {
		if ((ret = create(path, mode, kFileInode, &ino)) < 0) {
			return ret;
		}	
	} else {
		if ((ret = global_ipc_layer->call(FileSystemProtocol::kNamei, 
										  global_ipc_layer->id(), std::string(path), protocol_ino)) < 0) 
		{
			return -E_IPC;
		}
		if (ret > 0) {
			return -ret;
		}
		ino = protocol_ino;
	}
	fp->Init(ino, flags);

	return fd;
}


int
Client::Close(int fd)
{
	return global_fmgr->Put(fd);
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

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Write(src, n, offset);
}


int 
Client::ReadOffset(int fd, char* dst, uint64_t n, uint64_t offset)
{
	int   ret;
	File* fp;

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Read(dst, n, offset);
}


int 
Client::Write(int fd, const char* src, uint64_t n)
{
	File* fp;
	int   ret;

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Write(src, n);
}


int 
Client::Read(int fd, char* dst, uint64_t n)
{
	int   ret;
	File* fp;

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Read(dst, n);
}



uint64_t 
Client::Seek(int fd, uint64_t offset, int whence)
{
	int   ret;
	File* fp;

	if ((ret = global_fmgr->Lookup(fd, &fp)) < 0) {
		return ret;
	}
	return fp->Seek(offset, whence);
}


int
Client::CreateDir(const char* path, int mode)
{
	int         ret;
	InodeNumber ino;

	dbg_log (DBG_INFO, "Create Directory: %s\n", path);	

	if ((ret = create(path, mode, kDirInode, &ino)) < 0) {
		return ret;
	}
	return 0;
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
	//FIXME: save the path to cwd. we pass it to the server when we need a name resolution.
	//return global_namespace->SetCurWrkDir(session, path);
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
	int reply;
	int ret;

	if ((ret = global_ipc_layer->call(FileSystemProtocol::kLink, 
	                                  global_ipc_layer->id(), std::string(oldpath), std::string(newpath), reply)) < 0) 
	{
		return -E_IPC;
	}
	if (ret > 0) {
		return -ret;
	}
	return E_SUCCESS;
}


int 
Client::Unlink(const char* path)
{
	int reply;
	int ret;

	if ((ret = global_ipc_layer->call(FileSystemProtocol::kUnlink, 
	                                  global_ipc_layer->id(), std::string(path), reply)) < 0) 
	{
		return -E_IPC;
	}
	if (ret > 0) {
		return -ret;
	}
	return E_SUCCESS;
}


// current cfs synchronously writes data and metadata when doing 
// the call to the server
int 
Client::Sync()
{
	return E_SUCCESS;
}


// current cfs synchronously writes data and metadata when doing 
// the call to the server
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
