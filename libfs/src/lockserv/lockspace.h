#ifndef _LOCKSPACE_H_JKL121
#define _LOCKSPACE_H_JKL121

#include <pthread.h>

enum {
	LS_VOLATILE = 1
};

class LockSpace {
public:
	LockSpace();
	int Init(char* lockspace_name, int flags, int num_locks);
	void Lock(int lock_id);
	void Unlock(int lock_id);
	void Destroy();

private:
	void*            _mmap_ptr;
	size_t           _mmap_len;
    pthread_mutex_t* _mutexp;
    unsigned int     _num_locks;
};


struct LockSpaceHeader {
	unsigned int _magic;
	unsigned int _num_locks;
};
typedef struct LockSpaceHeader LockSpaceHeader;

#endif /* _LOCKSPACE_H_JKL121 */
