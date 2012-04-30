// The C front API of the library.
//
// This API helps the user to use both kernel and library file systems 
// transparently.

#ifndef __STAMNOS_RXFS_CLIENT_C_FRONT_API
#define __STAMNOS_RXFS_CLIENT_C_FRONT_API

#include <sys/types.h>
#include <stdint.h>

#define RXFS_FRONTAPI(fname) rxfs_##fname

#ifdef __cplusplus 
extern "C" { 
#endif

struct RFile;


int RXFS_FRONTAPI(init) (int argc, char* argv[]);
int RXFS_FRONTAPI(init2) (const char* xdst);
int RXFS_FRONTAPI(init3) (const char* xdst, int debug_level);
int RXFS_FRONTAPI(shutdown) ();
int RXFS_FRONTAPI(open) (const char* path, int flags);
int RXFS_FRONTAPI(open2) (const char* path, int flags, mode_t mode);
int RXFS_FRONTAPI(close) (int fd);
int RXFS_FRONTAPI(dup) (int oldfd);
int RXFS_FRONTAPI(dup2) (int oldfd, int newfd);
int RXFS_FRONTAPI(mount) (const char* source, const char* target, const char* fstype, uint32_t flags);
int RXFS_FRONTAPI(umount) (const char* target);
int RXFS_FRONTAPI(link) (const char* oldpath, const char* newpath);
int RXFS_FRONTAPI(unlink) (const char* pathname);
int RXFS_FRONTAPI(mkdir) (const char* path, int mode);
int RXFS_FRONTAPI(rmdir) (const char* path);
int RXFS_FRONTAPI(chdir) (const char* path);
char* RXFS_FRONTAPI(getcwd) (char* path, size_t size);

ssize_t RXFS_FRONTAPI(read) (int fd, void *buf, size_t count);
ssize_t RXFS_FRONTAPI(write) (int fd, const void *buf, size_t count);
ssize_t RXFS_FRONTAPI(pread) (int fd, void *buf, size_t count, off_t offset);
ssize_t RXFS_FRONTAPI(pwrite) (int fd, const void *buf, size_t count, off_t offset);
off_t RXFS_FRONTAPI(lseek) (int fd, off_t offset, int whence);
int RXFS_FRONTAPI(stat) (const char *path, struct stat *buf);
int RXFS_FRONTAPI(sync) ();
int RXFS_FRONTAPI(fsync) (int fd);

struct RFile* RXFS_FRONTAPI(fopen) (const char* pathname, int flags);
int RXFS_FRONTAPI(fclose) (struct RFile* rfile);
ssize_t RXFS_FRONTAPI(fread) (struct RFile* rfile, void *buf, size_t count);
ssize_t RXFS_FRONTAPI(fpread) (struct RFile* rfile, void *buf, size_t count, off_t offset);

#ifdef __cplusplus 
} 
#endif


#endif /* __STAMNOS_RXFS_CLIENT_C_FRONT_API_H */
