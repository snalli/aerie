#ifndef _FS_H_KIA111
#define _FS_H_KIA111

#include <sys/types.h>

struct FileStream;

FileStream *fopen(const char *path, const char *mode)
int fclose(FileStream *fp);
size_t fread(void *ptr, size_t size, size_t nmemb,  FileStream *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FileStream *stream);

#endif
