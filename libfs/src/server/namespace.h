#ifndef _NAMESPACE_H_IKL111
#define _NAMESPACE_H_IKL111

#include <pthread.h>


class NameSpace {
	public:
		NameSpace();
		int Init();
		int Lookup(const char*, void**);
		int Mount(const char*, void*);
		int Unmount(const char*);
	private:
		pthread_mutex_t   mutex_;
};


#endif
