#include "rpc.fixture.h"
#include <pthread.h>

bool RPCFixture::initialized = false;
pthread_mutex_t RPCFixture::mutex = PTHREAD_MUTEX_INITIALIZER;;
