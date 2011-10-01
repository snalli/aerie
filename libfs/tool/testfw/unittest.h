#ifndef _TESTFW_UNITTEST_H_AFS156
#define _TESTFW_UNITTEST_H_AFS156

#include <iostream>
#include <fstream>
#include <getopt.h>
#include <string.h>
#include <unittest++/UnitTest++.h>
#include <unittest++/TestReporterStdout.h>
#include <unittest++/TestRunner.h>
#include <unittest++/Test.h>
#include <unittest++/XmlTestReporter.h>

struct RunTestIfTestNameIs
{
	RunTestIfTestNameIs(char const* test_name)
		: test_name_(test_name)
	{ }

	bool operator()(const UnitTest::Test* const test) const
	{
		using namespace std;
		return (0 == strcmp(test->m_details.testName, test_name_));
	}

	char const* test_name_;
};



struct RunTestIfMatchFilter
{
	RunTestIfMatchFilter(char const* suite_filter, char const* test_filter)
	: suite_filter_(suite_filter),
	  test_filter_(test_filter)
	{ }

	bool operator()(const UnitTest::Test* const test) const
	{
		int suite_filter_len;
		int test_filter_len;
		
		suite_filter_len = suite_filter_ ? strlen(suite_filter_): 0;
		test_filter_len = test_filter_ ? strlen(test_filter_): 0;
		if ((suite_filter_len == 0 || 
		     (strncmp(test->m_details.suiteName, suite_filter_, suite_filter_len) == 0)) &&
			(test_filter_len == 0 || 
		     (strncmp(test->m_details.testName, test_filter_, test_filter_len) == 0)))
		{
			return true;
		} 
		return false;
	}

	char const* suite_filter_;
	char const* test_filter_;
};


static inline int runTests(const char *suiteName, const char *testName)
{
	int                          ret;
	UnitTest::XmlTestReporter    reporter(std::cerr);
	UnitTest::TestRunner         runner(reporter);

	printf("suite=%s, test=%s\n", suiteName, testName);
	if (testName || suiteName) {
		RunTestIfMatchFilter predicate(suiteName, testName);
		return runner.RunTestsIf(UnitTest::Test::GetTestList(), NULL, predicate, 0);
	} else {
		return runner.RunTestsIf(UnitTest::Test::GetTestList(), NULL, UnitTest::True(), 0);
	}
}


static inline int getTest(int argc, char **argv, char **suiteName, char **testName)
{
	extern char  *optarg;
	extern int   optind;
	extern int   opterr;
	int          c;
	char         *_suiteName = NULL;
	char         *_testName = NULL;

	optind = 1;
	opterr = 0;
	while (1) {
		static struct option long_options[] = {
			{"suite",  required_argument, 0, 's'},
			{"test", required_argument, 0, 't'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
     
		c = getopt_long (argc, argv, "s:t:",
		                 long_options, &option_index);
     
		/* Detect the end of the options. */
		if (c == -1)
			break;
		switch (c) {
			case 's':
				_suiteName = optarg;
				break;
			case 't':
				_testName = optarg;
				break;
		}
	}

	*suiteName = _suiteName;
	*testName = _testName;

	if (_suiteName && _testName) {
		return 0;
	} else {
		return 1;
	}
		
}

//
// Macros for constructing multi-threaded unit tests
//

#define TEST_THREAD_FIXTURE_DECLARATION(Fixture, TestName, ThreadTestName)   \
class Fixture##TestName##Helper;                                             \
struct Fixture##TestName##ThreadTestName##Helper {                           \
	Fixture##TestName##Helper* shared;                                       \
	int                        thread_id;                                    \
};                                                                           \
void* ThreadTestName(Fixture##TestName##ThreadTestName##Helper* self);


#define TEST_THREAD_FIXTURE_DEFINITION(Fixture, TestName, ThreadTestName)    \
void* ThreadTestName(Fixture##TestName##ThreadTestName##Helper* self) 


#define CALL_AND_WAIT_THREADS(Fixture, TestName, ThreadTestName, NumThreads) \
do {                                                                         \
  pthread_t* th = new pthread_t[NumThreads];                                 \
                                                                             \
  Fixture##TestName##ThreadTestName##Helper* th_struct =                     \
        new Fixture##TestName##ThreadTestName##Helper[NumThreads];           \
                                                                             \
  for (int i=0; i<NumThreads; i++) {                                         \
    th_struct[i].thread_id = i;                                              \
    th_struct[i].shared = this;                                              \
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



#endif /* _TESTFW_UNITTEST_H_AFS156 */
