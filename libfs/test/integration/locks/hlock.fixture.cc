#include "hlock.fixture.h"
#include <pthread.h>

bool HLockFixture::initialized = false;
pthread_mutex_t HLockFixture::mutex = PTHREAD_MUTEX_INITIALIZER;;
