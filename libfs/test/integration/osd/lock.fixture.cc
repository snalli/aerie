#include "lock.fixture.h"
#include <pthread.h>

bool LockFixture::initialized = false;
pthread_mutex_t LockFixture::mutex = PTHREAD_MUTEX_INITIALIZER;;
