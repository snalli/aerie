#include "cfs.fixture.h"
#include <pthread.h>

bool CFSFixture::initialized = false;
pthread_mutex_t CFSFixture::mutex = PTHREAD_MUTEX_INITIALIZER;;
