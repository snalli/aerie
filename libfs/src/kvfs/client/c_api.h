// The C front API of the library.

#ifndef __STAMNOS_FS_CLIENT_C_FRONT_API
#define __STAMNOS_FS_CLIENT_C_FRONT_API

#include <sys/types.h>
#include <stdint.h>

#define FRONTAPI(fname) kvfs_##fname

#ifdef __cplusplus 
extern "C" { 
#endif


int FRONTAPI(init) (int argc, char* argv[]);
int FRONTAPI(init2) (const char* xdst);
int FRONTAPI(shutdown) ();
int FRONTAPI(mount) (const char* source, uint32_t flags);
int FRONTAPI(umount) ();
ssize_t FRONTAPI(get) (const char* key, void *buf);
ssize_t FRONTAPI(put) (const char* key, const void *buf, size_t count);
int FRONTAPI(sync) ();

#ifdef __cplusplus 
} 
#endif


#endif /* __STAMNOS_FS_CLIENT_C_FRONT_API_H */
