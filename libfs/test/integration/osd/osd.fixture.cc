#include "osd.fixture.h"
#include <pthread.h>

bool OsdFixture::initialized = false;
pthread_mutex_t OsdFixture::mutex = PTHREAD_MUTEX_INITIALIZER;;
