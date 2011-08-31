#include "lockspace.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

LockSpace::LockSpace()
{

}

int 
LockSpace::Init(char* lockspace_name, int flags, int num_locks)
{
	int                 i;
	int                 fd;
	int                 ret;
	size_t              length;
	pthread_mutexattr_t psharedm_attr;
	LockSpaceHeader     header;
	pthread_mutex_t*    mutexp;
	char*               tmpfilename;
	void*               ptr;

	length = sizeof(pthread_mutex_t) * num_locks + sizeof(LockSpaceHeader);

	if ((fd = open(lockspace_name, O_RDWR)) < 0) {
		/* Atomically create and initialize a lock file */
		do {
			tmpfilename = tempnam("/tmp", NULL);
			fd = open(tmpfilename, O_RDWR | O_CREAT | O_EXCL, 0666);
		} while (fd < 0 && errno == EEXIST);
		if (fd < 0) {
			return -1;
		}
		ret = ftruncate(fd, length);
		if (ret < 0) {
			close (fd);
			return ret;
		}
		ptr = mmap(NULL, length,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				fd, 0);
		mutexp = (pthread_mutex_t*) ((char*) ptr + sizeof(LockSpaceHeader));
    	close (fd);

		pthread_mutexattr_init(&psharedm_attr);
		pthread_mutexattr_setpshared(&psharedm_attr, PTHREAD_PROCESS_SHARED);
		for (i=0; i<num_locks; i++) {
		    pthread_mutex_init(&mutexp[i], &psharedm_attr);
		}
		if ((ret = link(tmpfilename, lockspace_name)) < 0) {
			if (errno == EEXIST) {
			}
		}
		/* We unmap and remap below to ensure that the mapped lock file appears 
		 * in the address space named as lockspace_name. This simplifies locating
		 * where the lock file is mapped in our address space and hence makes 
		 * debugging easier.
		 */ 
		munmap((void *) ptr, length);
		unlink(tmpfilename);
		fd = open(lockspace_name, O_RDWR);
	}

	/* we map the lock file */
	ptr = mmap(NULL, length,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, 0);
	mutexp = (pthread_mutex_t*) ((char*) ptr + sizeof(LockSpaceHeader));
	close(fd);

	_num_locks = num_locks;
	_mutexp = mutexp;
	_mmap_len = length;
	_mmap_ptr = ptr;

	return 0;
}


void 
LockSpace::Lock(int lock_id)
{
	pthread_mutex_t* mutex;

	mutex = &_mutexp[lock_id];
    pthread_mutex_lock(mutex);
}


void 
LockSpace::Unlock(int lock_id)
{
	pthread_mutex_t* mutex;

	mutex = &_mutexp[lock_id];
    pthread_mutex_unlock(mutex);
}


void
LockSpace::Destroy()
{
	int length = _num_locks * sizeof(*_mutexp);
    munmap((void *) _mmap_ptr, _mmap_len);
}
