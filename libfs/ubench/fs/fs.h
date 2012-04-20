#ifndef __STAMNOS_UBENCH_FS_H
#define __STAMNOS_UBENCH_FS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern int (*fs_open)(const char*, int flags);
extern int (*fs_open2)(const char*, int flags, mode_t mode);
extern int (*fs_unlink)(const char*);
extern int (*fs_close)(int);
extern int (*fs_fsync)(int);
extern int (*fs_sync)();

extern int ubench_fs_open(int argc, char* argv[]);
extern int ubench_fs_create(int argc, char* argv[]);
extern int ubench_fs_unlink(int argc, char* argv[]);


#endif // __STAMNOS_UBENCH_FS_H
