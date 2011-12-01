#include "mfs.fixture.h"
#include <pthread.h>

bool MFSFixture::initialized = false;
pthread_mutex_t MFSFixture::mutex = PTHREAD_MUTEX_INITIALIZER;;
