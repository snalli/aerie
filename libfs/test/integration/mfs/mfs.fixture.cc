#include "mfs.fixture.h"
#include <pthread.h>
#include <string>
#include "fileset.h"

FileSet MFSFixture::fileset_exists;
FileSet MFSFixture::fileset_notexists;
bool MFSFixture::initialized = false;
pthread_mutex_t MFSFixture::mutex = PTHREAD_MUTEX_INITIALIZER;;


int MakeDir(const char* path)
{
	std::stringstream   ss_root;
	char*               token;
	char                buf2[1024];
	int                 ret;

	strcpy(buf2, path);
	token = strtok(buf2, "/");
	ss_root << "/";
	ss_root << std::string(token);
	while ((token=strtok(NULL, "/")) != NULL) {
		ss_root << "/";
		ss_root << std::string(token);
		ret = libfs_mkdir(ss_root.str().c_str(), S_IRUSR|S_IWUSR|S_IXUSR);
	}
	return ret;
}


int MakeFile(const char* path) 
{
	int         ret;
	std::string dir = ExtractDirectory(std::string(path));

	MakeDir(dir.c_str());
	if ((ret = libfs_open(path, O_CREAT|O_RDWR)) < 0) {
		return ret;
	}
	if ((ret = libfs_close(ret)) < 0) {
		return ret;
	}
	return ret;
}

	
int 
MapFileSystemImage(MFSFixture* fixture, Session* session, testfw::Test* test, const char* root, int nfiles, int dirwidth, int dirdepth)
{
	int               dirfanout;
	int               k;
	int               n;
	FileSet::iterator itr;

	srand(0); // so that different invocations give the same result

	dirfanout = 0;
	do { 
		dirfanout++;
		k = dirwidth * pow(dirfanout, dirdepth) - dirfanout*nfiles - dirwidth - nfiles;
	} while (k < 0);	
	
	if (strcmp(test->Tag(), "C1:T1") == 0 || strcmp(test->Tag(), "C2:T1") == 0) {
		FileSet::Create(root, dirwidth, dirdepth, dirfanout, &fixture->fileset_exists);
		fixture->fileset_exists.MoveRandom(fixture->fileset_exists.Size() / 2, 
                                                   &fixture->fileset_notexists);
	}
	if (strcmp(test->Tag(), "C1:T1") == 0) {
		for (itr = fixture->fileset_exists.begin(); itr != fixture->fileset_exists.end(); itr++) {
			MakeFile(itr->c_str());
		}
		libfs_sync();
	}
	return 0;
}
