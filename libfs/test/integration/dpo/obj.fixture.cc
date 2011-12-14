#include "obj.fixture.h"
#include <pthread.h>

bool ObjectFixture::initialized = false;
pthread_mutex_t ObjectFixture::mutex = PTHREAD_MUTEX_INITIALIZER;;
