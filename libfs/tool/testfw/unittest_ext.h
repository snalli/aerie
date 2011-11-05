#ifndef _TESTFW_UNITTEST_EXTENSION_H_AFS156
#define _TESTFW_UNITTEST_EXTENSION_H_AFS156


//
// Macros for constructing multi-threaded unit tests
// 
// DEPRECATED: We deprecate the following macros for two reasons:
// 1) These macros create threads inside a unittest. Since the 
//    unittest++ library is not thread-safe, we suffer from races. 
// 2) Threads and clients have to execute the same unit-test. We 
//    prefer to be able to run different unittest under each thread/client
//    so that we can combine unittests together
//

#if 0 // DEPRECATED

#include "tool/testfw/ut_barrier.h"

#define TEST_THREAD_FIXTURE_DECLARATION(Fixture, TestName, ThreadTestName)   \
class Fixture##TestName##Helper;                                             \
struct Fixture##TestName##ThreadTestName##Helper {                           \
	Fixture##TestName##Helper* shared;                                       \
	ut_barrier_t*              barrier;                                      \
	int                        thread_id;                                    \
};                                                                           \
void* ThreadTestName(Fixture##TestName##ThreadTestName##Helper* self);


#define TEST_THREAD_FIXTURE_DEFINITION(Fixture, TestName, ThreadTestName)    \
void* ThreadTestName(Fixture##TestName##ThreadTestName##Helper* self) 


#define CALL_AND_WAIT_THREADS(Fixture, TestName, ThreadTestName, NumThreads) \
do {                                                                         \
  pthread_t*   th = new pthread_t[NumThreads];                               \
  ut_barrier_t thread_barrier;												 \
                                                                             \
  Fixture##TestName##ThreadTestName##Helper* th_struct =                     \
        new Fixture##TestName##ThreadTestName##Helper[NumThreads];           \
                                                                             \
  ut_barrier_init(&thread_barrier, NumThreads, false);                       \
  for (int i=0; i<NumThreads; i++) {                                         \
    th_struct[i].thread_id = i;                                              \
    th_struct[i].shared = this;                                              \
	th_struct[i].barrier = &thread_barrier;                                  \
    pthread_create(&th[i], NULL, (void* (*)(void*))ThreadTestName,           \
                   &th_struct[i]);                                           \
  }                                                                          \
                                                                             \
  for (int i=0; i<NumThreads; i++) {                                         \
    pthread_join(th[i], NULL);                                               \
  }                                                                          \
                                                                             \
  delete []th;\
} while(0);


#define TEST_THREAD_LOCAL (self)
#define TEST_THREAD_SHARED ((self)->shared)


#define TEST_THREAD_FIXTURE(Fixture, TestName, NumThreads)                   \
  TEST_THREAD_FIXTURE_DECLARATION(Fixture, TestName, TestName##Thread)       \
  TEST_FIXTURE(Fixture, TestName)                                            \
  {                                                                          \
    CALL_AND_WAIT_THREADS(Fixture, TestName, TestName##Thread, NumThreads);  \
  }                                                                          \
                                                                             \
  TEST_THREAD_FIXTURE_DEFINITION(Fixture, TestName, TestName##Thread)

#endif 

#endif /* _TESTFW_UNITTEST_EXTENSION_H_AFS156 */
