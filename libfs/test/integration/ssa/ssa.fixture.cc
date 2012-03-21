#include "ssa.fixture.h"
#include <pthread.h>

bool SsaFixture::initialized = false;
pthread_mutex_t SsaFixture::mutex = PTHREAD_MUTEX_INITIALIZER;;
