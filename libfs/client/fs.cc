#include "fs.h"
#include <string.h>

typedef struct {
	char name[16][16];
	int  idx[16];
} FsSuperblock;


struct FileStream_ {
	char   path[128];
	size_t offset;
};


int mkdir()
{

}


int chdir()
{

}


FileStream *
fopen(const char *path, const char *mode)
{
	FileStream *stream;

	stream = new FileStream();
	strcpy(stream->path, path);
}


int 
fclose(FileStream *fp)
{
	delete fp;
}


size_t 
fread(void *ptr, size_t size, size_t nmemb,  FileStream *stream);
{


}


size_t 
fwrite(const void *ptr, size_t size, size_t nmemb, FileStream *stream)
{


}
