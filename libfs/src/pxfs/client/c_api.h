// The C front API of the library.
//
// This API helps the user to use both kernel and library file systems 
// transparently.

#ifndef __STAMNOS_FS_CLIENT_C_FRONT_API
#define __STAMNOS_FS_CLIENT_C_FRONT_API

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#define PXFS_FRONTAPI(fname) libfs_##fname

#ifdef __cplusplus 
extern "C" { 
#endif


int PXFS_FRONTAPI(init) (int argc, char* argv[]);
int PXFS_FRONTAPI(init2) (const char* xdst);
int PXFS_FRONTAPI(init3) (const char* xdst, int debug_level);
int PXFS_FRONTAPI(shutdown) ();
int PXFS_FRONTAPI(open) (const char* path, int flags);
int PXFS_FRONTAPI(open2) (const char* path, int flags, mode_t mode);
int PXFS_FRONTAPI(close) (int fd);
int PXFS_FRONTAPI(dup) (int oldfd);
int PXFS_FRONTAPI(dup2) (int oldfd, int newfd);
int PXFS_FRONTAPI(mount) (const char* source, const char* target, const char* fstype, uint32_t flags);
int PXFS_FRONTAPI(umount) (const char* target);
int PXFS_FRONTAPI(link) (const char* oldpath, const char* newpath);
int PXFS_FRONTAPI(unlink) (const char* pathname);
int PXFS_FRONTAPI(rename) (const char* oldpath, const char* newpath);
int PXFS_FRONTAPI(mkdir) (const char* path, int mode);
int PXFS_FRONTAPI(rmdir) (const char* path);
int PXFS_FRONTAPI(chdir) (const char* path);
char* PXFS_FRONTAPI(getcwd) (char* path, size_t size);

ssize_t PXFS_FRONTAPI(read) (int fd, void *buf, size_t count);
ssize_t PXFS_FRONTAPI(write) (int fd, const void *buf, size_t count);
ssize_t PXFS_FRONTAPI(pread) (int fd, void *buf, size_t count, off_t offset);
ssize_t PXFS_FRONTAPI(pwrite) (int fd, const void *buf, size_t count, off_t offset);
off_t PXFS_FRONTAPI(lseek) (int fd, off_t offset, int whence);
int PXFS_FRONTAPI(stat) (const char *path, struct stat *buf);
int PXFS_FRONTAPI(sync) ();
int PXFS_FRONTAPI(fsync) (int fd);


#ifdef __cplusplus 
} 
#endif


#endif /* __STAMNOS_FS_CLIENT_C_FRONT_API_H */
