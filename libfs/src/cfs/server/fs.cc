#include <string>
#include "common/errno.h"
#include "bcs/bcs.h"
#include "osd/main/server/osd.h"
#include "osd/main/server/sessionmgr.h"
#include "scm/pool/pool.h"
#include "cfs/common/fs_protocol.h"
#include "cfs/server/fs.h"
#include "cfs/server/session.h"
#include "cfs/server/server.h"
#include "cfs/server/sb.h"
#include "cfs/server/namespace.h"
#include "cfs/server/dir_inode.h"
#include "cfs/server/file_inode.h"
#include "cfs/common/shreg.h"


namespace server {

FileSystem::FileSystem(Ipc* ipc, StorageSystem* storage_system)
	: ipc_(ipc),
	  storage_system_(storage_system)
{
	pthread_mutex_init(&mutex_, NULL);
}


int
FileSystem::Init()
{
	int ret;

	sb_ = new SuperBlock(storage_system_->super_obj());
	if (ipc_) {
		if ((ret = ipc_handlers_.Register(this)) < 0) {
			return ret;
		}
	}
	return E_SUCCESS;
}


int 
FileSystem::Make(const char* target, size_t nblocks, size_t block_size, int flags) 
{
	int ret;

	if ((ret = StorageSystem::Make(target, flags)) < 0) {
		return ret;
	}
	return E_SUCCESS;
}


int 
FileSystem::Load(Ipc* ipc, const char* source, unsigned int flags, FileSystem** fsp)
{
	int            ret;
	StorageSystem* storage_system;
	FileSystem*    fs;

	if ((ret = StorageSystem::Load(ipc, source, flags, &storage_system)) < 0) {
		return ret;
	}
	if ((fs = new FileSystem(ipc, storage_system)) == NULL) {
		storage_system->Close();
		return -E_NOMEM;
	}
	if ((ret = fs->Init()) < 0) {
		return ret;
	}
	*fsp = fs;
	return E_SUCCESS;
}


int
FileSystem::MakeDir(Session* session, const char* path, 
                    unsigned int mode,
                    FileSystemProtocol::InodeNumber& ino)
{
	int         ret;
	char        name[128];
	DirInode    dinode1;
	DirInode    dinode2;
	InodeNumber parino;
	InodeNumber childino;

	dbg_log (DBG_INFO, "Create directory %s\n", path);
	
	
	if ((ret = session->namespace_->Nameiparent(session, path, name, &parino)) < 0) {
		return ret;
	}
	
	DirInode* pp = DirInode::Load(session, parino, &dinode1);
	DirInode* cp = DirInode::Make(session, &dinode2);
	if ((ret = pp->Link(session, name, cp)) < 0) { return ret; }
	if ((ret = cp->Link(session, ".", cp)) < 0) { return ret; }
	if ((ret = cp->Link(session, "..", pp)) < 0) { return ret; }

	ino = cp->ino();

	return E_SUCCESS;
}


int
FileSystem::MakeFile(Session* session, const char* path, 
                     unsigned int flags, unsigned int mode,
                     FileSystemProtocol::InodeNumber& ino)
{
	int         ret;
	char        name[128];
	DirInode    dinode1;
	DirInode    dinode2;
	FileInode   finode;
	InodeNumber parino;
	InodeNumber childino;
	
	dbg_log (DBG_INFO, "Create file %s\n", path);
	if ((ret = session->namespace_->Nameiparent(session, path, name, &parino)) < 0) {
		return ret;
	}
	
	DirInode* pp = DirInode::Load(session, parino, &dinode1);
	FileInode* cp = FileInode::Make(session, &finode);
	if ((ret = pp->Link(session, name, cp)) < 0) {
		return ret;
	}
	ino = cp->ino();

	return E_SUCCESS;
}


int
FileSystem::Read(Session* session, FileSystemProtocol::InodeNumber ino, 
                 void* buf, int count, int offset, int& n)
{
	int         ret;
	char        name[128];
	FileInode   finode;
	InodeNumber fino = ino;
	
	dbg_log (DBG_INFO, "Read file inode %lx\n", fino);
	
	FileInode* fp = FileInode::Load(session, fino, &finode);
	n = fp->Read(session, buf, count, offset);
	return E_SUCCESS;
}


int
FileSystem::Write(Session* session, FileSystemProtocol::InodeNumber ino, 
                  void* buf, int count, int offset, int& n)
{
	int         ret;
	char        name[128];
	FileInode   finode;
	InodeNumber fino = ino;
	
	dbg_log (DBG_INFO, "Write file inode %lx\n", fino);
	
	FileInode* fp = FileInode::Load(session, fino, &finode);
	n = fp->Write(session, buf, count, offset);
	return E_SUCCESS;
}


int
FileSystem::Link(Session* session, const char* oldpath, const char* newpath)
{
	int         ret;
	char        newname[128];
	DirInode    dinode1;
	InodeNumber parino;
	InodeNumber oldino;
	
	dbg_log (DBG_INFO, "Link %s -> %s\n", newpath, oldpath);
	
	if ((ret = session->namespace_->Nameiparent(session, newpath, newname, &parino)) < 0) {
		return ret;
	}
	if ((ret = session->namespace_->Namei(session, oldpath, &oldino)) < 0) {
		return ret;
	}
	DirInode* dp = DirInode::Load(session, parino, &dinode1);
	if (dp == NULL) {
		return -E_NOENT;
	}
	return dp->Link(session, newname, oldino);
}


int
FileSystem::Unlink(Session* session, const char* path)
{
	int         ret;
	char        name[128];
	DirInode    dinode1;
	DirInode    dinode2;
	FileInode   finode;
	InodeNumber parino;
	InodeNumber childino;
	
	dbg_log (DBG_INFO, "Unlink %s\n", path);
	
	if ((ret = session->namespace_->Nameiparent(session, path, name, &parino)) < 0) {
		return ret;
	}
	DirInode* dp = DirInode::Load(session, parino, &dinode1);
	if (dp == NULL) {
		return -E_NOENT;
	}
	return  dp->Unlink(session, name);
}


int
FileSystem::Namei(Session* session, const char* path, FileSystemProtocol::InodeNumber& ino)
{
	int         ret;
	char        name[128];
	InodeNumber target_ino;
	
	dbg_log (DBG_INFO, "Path name to inode number %s\n", path);
	
	if ((ret = session->namespace_->Namei(session, path, &target_ino)) < 0) {
		return ret;
	}
	ino = target_ino;

	return E_SUCCESS;
}


int 
FileSystem::Mount(int clt, const char* source, const char* target, 
                  unsigned int flags, FileSystemProtocol::MountReply& rep) 
{
	int                               ret;
	StorageSystemProtocol::MountReply ssrep;
	::server::BaseSession*            basesession;
	::server::Session*                session;
	
	if ((ret = storage_system_->Mount(clt, source, flags, ssrep)) < 0) {
		return ret;
	}
	rep.desc_ = ssrep.desc_;
	
	if ((ret = ipc_->session_manager()->Lookup(clt, &basesession)) < 0) {
		return -ret;
	}
	session = static_cast<Session*>(basesession);
	session->namespace_ = new NameSpace(sb_->root_ino());
	session->shreg_ = new SharedRegion(clt);
	assert(session->shreg_->Create() == 0);

	return E_SUCCESS;
}


int 
FileSystem::IpcHandlers::Register(FileSystem* module)
{
	module_ = module;
	module_->ipc_->reg(::FileSystemProtocol::kMount, this, 
	                   &::server::FileSystem::IpcHandlers::Mount);
	module_->ipc_->reg(::FileSystemProtocol::kMakeDir, this, 
	                   &::server::FileSystem::IpcHandlers::MakeDir);
	module_->ipc_->reg(::FileSystemProtocol::kMakeFile, this, 
	                   &::server::FileSystem::IpcHandlers::MakeFile);
	module_->ipc_->reg(::FileSystemProtocol::kWrite, this, 
	                   &::server::FileSystem::IpcHandlers::Write);
	module_->ipc_->reg(::FileSystemProtocol::kRead, this, 
	                   &::server::FileSystem::IpcHandlers::Read);
	module_->ipc_->reg(::FileSystemProtocol::kLink, this, 
	                   &::server::FileSystem::IpcHandlers::Link);
	module_->ipc_->reg(::FileSystemProtocol::kUnlink, this, 
	                   &::server::FileSystem::IpcHandlers::Unlink);
	module_->ipc_->reg(::FileSystemProtocol::kNamei, this, 
	                   &::server::FileSystem::IpcHandlers::Namei);

	return E_SUCCESS;
}


int
FileSystem::IpcHandlers::MakeDir(unsigned int clt, std::string path, 
                                 unsigned int mode,
                                 FileSystemProtocol::InodeNumber& ino)
{
	int                    ret;
	::server::BaseSession* basesession;
	::server::Session*     session;
	if ((ret = module_->ipc_->session_manager()->Lookup(clt, &basesession)) < 0) {
		return -ret;
	}
	session = static_cast<Session*>(basesession);

	if ((ret = module_->MakeDir(session, path.c_str(), mode, ino)) < 0) {
		return -ret;
	}
	return 0;
}


int
FileSystem::IpcHandlers::MakeFile(unsigned int clt, std::string path, 
                                  unsigned int flags, unsigned int mode,
                                  FileSystemProtocol::InodeNumber& ino)
{
	int                    ret;
	::server::BaseSession* basesession;
	::server::Session*     session;
	
	if ((ret = module_->ipc_->session_manager()->Lookup(clt, &basesession)) < 0) {
		return -ret;
	}
	session = static_cast<Session*>(basesession);

	if ((ret = module_->MakeFile(session, path.c_str(), flags, mode, ino)) < 0) {
		return -ret;
	}
	return 0;
}


int
FileSystem::IpcHandlers::Read(unsigned int clt, FileSystemProtocol::InodeNumber ino, int count, int offset, int& r)
{
	int                    ret;
	::server::BaseSession* basesession;
	::server::Session*     session;
	void*                  buf; 
	
	if ((ret = module_->ipc_->session_manager()->Lookup(clt, &basesession)) < 0) {
		return -ret;
	}
	session = static_cast<Session*>(basesession);

	buf = session->shreg_->base();
	if ((ret = module_->Read(session, ino, buf, count, offset, r)) < 0) {
		return -ret;
	}
	return 0;
}


int
FileSystem::IpcHandlers::Write(unsigned int clt, FileSystemProtocol::InodeNumber ino, int count, int offset, int& r)
{
	int                    ret;
	::server::BaseSession* basesession;
	::server::Session*     session;
	void*                  buf;
	
	if ((ret = module_->ipc_->session_manager()->Lookup(clt, &basesession)) < 0) {
		return -ret;
	}
	session = static_cast<Session*>(basesession);

	buf = session->shreg_->base();
	if ((ret = module_->Write(session, ino, buf, count, offset, r)) < 0) {
		return -ret;
	}
	return 0;
}


int
FileSystem::IpcHandlers::Link(unsigned int clt, std::string oldpath, std::string newpath, int& r)
{
	int                    ret;
	::server::BaseSession* basesession;
	::server::Session*     session;
	
	if ((ret = module_->ipc_->session_manager()->Lookup(clt, &basesession)) < 0) {
		return -ret;
	}
	session = static_cast<Session*>(basesession);

	if ((ret = module_->Link(session, oldpath.c_str(), newpath.c_str())) < 0) {
		return -ret;
	}
	return 0;
}


int
FileSystem::IpcHandlers::Unlink(unsigned int clt, std::string path, int& r)
{
	int                    ret;
	::server::BaseSession* basesession;
	::server::Session*     session;
	
	if ((ret = module_->ipc_->session_manager()->Lookup(clt, &basesession)) < 0) {
		return -ret;
	}
	session = static_cast<Session*>(basesession);

	if ((ret = module_->Unlink(session, path.c_str())) < 0) {
		return -ret;
	}
	return 0;
}


int
FileSystem::IpcHandlers::Namei(unsigned int clt, std::string path, 
                               FileSystemProtocol::InodeNumber& ino)
{
	int                    ret;
	::server::BaseSession* basesession;
	::server::Session*     session;
	
	if ((ret = module_->ipc_->session_manager()->Lookup(clt, &basesession)) < 0) {
		return -ret;
	}
	session = static_cast<Session*>(basesession);

	if ((ret = module_->Namei(session, path.c_str(), ino)) < 0) {
		return -ret;
	}
	return 0;
}


int
FileSystem::IpcHandlers::Mount(unsigned int clt, std::string source, 
                               std::string target, unsigned int flags,
                               FileSystemProtocol::MountReply& rep)
{
	int ret;
	
	if ((ret = module_->Mount(clt, source.c_str(), target.c_str(), 
	                          flags, rep)) < 0) {
		return -ret;
	}
	return 0;
}


} // namespace server
