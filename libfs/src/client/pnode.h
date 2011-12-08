#ifndef __STAMNOS_FS_CLIENT_PNODE_PROXY_H_
#define __STAMNOS_FS_CLIENT_PNODE_PROXY_H_

class PnodeProxy {

	int nlink() { printf("Inode: %llu, nlink_ = %d\n", ino_, nlink_); return nlink_; }
	int set_nlink(int nlink) { printf("Inode: %llu, set_nlink_ = %d\n", ino_, nlink); nlink_ = nlink; return 0;}


	//! hard link count
	int                 nlink_;

};

} // namespace client

#endif // __STAMNOS_FS_CLIENT_PNODE_PROXY_H_
