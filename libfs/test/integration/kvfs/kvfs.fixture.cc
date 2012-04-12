#include "kvfs.fixture.h"
#include <pthread.h>

bool KVFSFixture::initialized = false;
pthread_mutex_t KVFSFixture::mutex = PTHREAD_MUTEX_INITIALIZER;;
