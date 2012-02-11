#include "ipc.fixture.h"
#include <pthread.h>

bool IPCFixture::initialized = false;
pthread_mutex_t IPCFixture::mutex = PTHREAD_MUTEX_INITIALIZER;;
