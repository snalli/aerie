#include <sys/types.h>

struct buf;
struct file;
struct inode;
struct proc;
struct spinlock;
struct stat;

#include "types.h"

// bio.c
void            binit(void);
struct buf*     bread(uint, uint64);
void            brelse(struct buf*);
void            bwrite(struct buf*);

// file.c
struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
void            fileinit(void);
int             fileread(struct file*, char*, int n);
int             filestat(struct file*, struct stat*);
int             filewrite(struct file*, char*, int n);

// fs.c
int             dirlink(struct inode*, char*, uint);
struct inode*   dirlookup(struct inode*, char*, uint*);
struct inode*   ialloc(uint, short);
struct inode*   idup(struct inode*);
void            iinit(void);
void            ilock(struct inode*);
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             namecmp(const char*, const char*);
struct inode*   namei(char*);
struct inode*   nameiparent(char*, char*);
int             readi(struct inode*, char*, uint, uint);
void            stati(struct inode*, struct stat*);
int             writei(struct inode*, char*, uint, uint);

// scm.c
void            scmrw(struct buf*);


// spinlock.c
void            acquire(struct spinlock*);
void            initlock(struct spinlock*, char*);
void            release(struct spinlock*);

// util.c
void            panic(char *);
void            my_sleep(void *, struct spinlock *);
void            my_wakeup(void *);

// sysfile.c

int             libfs_open(char *path, int flags, int omode);
int             libfs_write(int fd, char *buf, int n);
int             libfs_read(int fd, char *buf, int n);
int             libfs_unlink(char *path);
int             libfs_mkdir(char *path, int mode);
char *          libfs_getwd(char *buf);


// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))


#undef SCMALLOC
#define SCMALLOC
