#ifndef _TESTFW_TESTFW_H_APK199
#define _TESTFW_TESTFW_H_APK199

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <set>
#include <stack>
#include <unittest++/UnitTest++.h>
#include <unittest++/TestReporterStdout.h>
#include <unittest++/TestRunner.h>
#include <unittest++/Test.h>
#include <unittest++/XmlTestReporter.h>
#include "tool/testfw/unittest.h"
#include "tool/testfw/argvmap.h"

#define TESTFW (testfw::__testfwp)
#define SELF (TESTFW->tid2Test(pthread_self()))
#define EVENT(event) ((TESTFW->tid2Test(pthread_self()))->Event(event))
#define TAG_DELIM ":"

namespace testfw {

class TestFramework;
extern TestFramework* __testfwp;

// Predicate filters 

struct RunTestIfPrefixMatchFilter
{
	RunTestIfPrefixMatchFilter(char const* suite_filter, char const* test_filter)
	: suite_filter_(suite_filter),
	  test_filter_(test_filter)
	{ }

	bool operator()(const UnitTest::Test* const test) const
	{
		int suite_filter_len;
		int test_filter_len;
		
		//std::cout << "TEST: " << test->m_details.suiteName << "/" << test->m_details.testName << std::endl;
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


struct RunTestIfExactMatchFilter
{
	RunTestIfExactMatchFilter(char const* suite_filter, char const* test_filter)
	: suite_filter_(suite_filter),
	  test_filter_(test_filter)
	{ }

	bool operator()(const UnitTest::Test* const test) const
	{
		int suite_filter_len;
		int test_filter_len;
		
		//std::cout << "EXACT MATCH: " << test->m_details.suiteName << "/" << test->m_details.testName << std::endl;
		if (((suite_filter_ && (strcmp(test->m_details.suiteName, suite_filter_) == 0)) ||
		     !suite_filter_) &&
		    ((test_filter_ && (strcmp(test->m_details.testName, test_filter_) == 0)) ||
		     !test_filter_))
		{
			return true;
		} 
		return false;
	}

	char const* suite_filter_;
	char const* test_filter_;
};


// Test descriptor

class Test {
	friend class TestFramework;
public:
	Test(ArgValMap* global, ArgValMap* local);
	static void* ThreadEntryPoint(void* arg);
	int Event(const char* event);
	const char* Tag() { return tag_name_; }

private:
	pthread_t            thread_;
	int                  msgid_;
	int                  semid_;
	bool                 deferred_;
	const char*          filter_type_;
	const char*          suite_name_;  
	const char*          test_name_;  
	const char*          tag_name_;  
	std::stringstream    oss_;
	std::string          tag_name_str_;
};


inline
Test::Test(ArgValMap* global, ArgValMap* local)
	: suite_name_(NULL),
	  test_name_(NULL),
	  tag_name_(NULL),
	  filter_type_(NULL),
	  semid_(-1),
	  msgid_(-1),
	  oss_(std::ostringstream::out)
{
	std::string* val;

#if __DEBUG
	if (global) {
		std::cout << "GLOBAL\n" << std::endl;
		global->Print(std::cout);
	}
	if (local) {
		std::cout << "LOCAL\n" << std::endl;
		local->Print(std::cout);
	}
#endif		

	if (global) {
		if (val = global->ArgVal(std::string("tag"))) {
			tag_name_ = val->c_str();
		}
		if (val = global->ArgVal(std::string("filter"))) {
			filter_type_ = val->c_str();
		}
		if (val = global->ArgVal(std::string("deferred"))) {
			deferred_ = true;
		} else {
			deferred_ = false;
		}
	}

	if (global && !local) {
		if (val = global->ArgVal(std::string("test"))) {
			test_name_ = val->c_str();
		}

		if (val = global->ArgVal(std::string("suite"))) {
			suite_name_ = val->c_str();
		}
	}

	// merge local 
	if (local) {
		if (val = local->ArgVal(std::string("test"))) {
			test_name_ = val->c_str();
		}

		if (val = local->ArgVal(std::string("suite"))) {
			suite_name_ = val->c_str();
		}
		
		if (val = local->ArgVal(std::string("msg"))) {
			key_t msg_key = atoi(val->c_str());
			msgid_ = msgget(msg_key, 0);
		}

		if (val = local->ArgVal(std::string("sem"))) {
			key_t sem_key = atoi(val->c_str());
			semid_ = semget(sem_key, 1, 0);
		}

		if (val = local->ArgVal(std::string("tag"))) {
			if (tag_name_) {
				tag_name_str_ = std::string(tag_name_) + TAG_DELIM;
			}
			tag_name_str_ += *val;
			tag_name_ = tag_name_str_.c_str();
		}
	}
}


inline
void* Test::ThreadEntryPoint(void* arg)
{
	int ret;
	Test* t = (Test*) arg;
	UnitTest::TestReporter* reporter;

	if (t->deferred_) {
		reporter = new UnitTest::XmlTestReporter(t->oss_);
	} else {
		reporter = new UnitTest::TestReporterStdout();
	}

	UnitTest::TestRunner runner(*reporter);

	if (t->suite_name_ || t->test_name_) {
		if (t->filter_type_) {
			if (strcmp(t->filter_type_, "prefix") == 0) {
				RunTestIfPrefixMatchFilter predicate(t->suite_name_, t->test_name_);
				ret = runner.RunTestsIf(UnitTest::Test::GetTestList(), NULL, predicate, 0);
			} else if (strcmp(t->filter_type_, "exact") == 0) {
				RunTestIfExactMatchFilter predicate(t->suite_name_, t->test_name_);
				ret = runner.RunTestsIf(UnitTest::Test::GetTestList(), NULL, predicate, 0);
			} else {
				// unknown filter: panic?
			}
		} else {
			// if no filter type provided then use exact match
			RunTestIfExactMatchFilter predicate(t->suite_name_, t->test_name_);
			ret = runner.RunTestsIf(UnitTest::Test::GetTestList(), NULL, predicate, 0);
		}
	} else {
		ret = runner.RunTestsIf(UnitTest::Test::GetTestList(), NULL, UnitTest::True(), 0);
	}
	return (void*) ret;
}

inline int 
Test::Event(const char* event)
{
	char buf[512];
	struct sembuf sb = {0, -1, 0};
	struct msgbuf* mb = (struct msgbuf*) &buf;

	if (msgid_ > 0 && semid_ > 0) {
		mb->mtype = 1;
		sprintf(mb->mtext, "%s%s%s", tag_name_, TAG_DELIM, event);
		msgsnd(msgid_, mb, strlen(mb->mtext), 0);
		semop(semid_, &sb, 1);
		return 0;
	}
	return -1;
}


// Test framework

struct AbstractFunctor {
	virtual void operator()() = 0;
};


class TestFramework {
public:
	TestFramework(int, char**);
	
	inline int RunTests();
	int ArgVal(std::string& key, std::string& val);
	int ArgVal(const char*, std::string& val);
	int numthreads() { return numthreads_; }
	int ParseArg(int argc, char** argv);
	Test* tid2Test(pthread_t tid);
	void RegisterFinalize(AbstractFunctor* functor);

private:
	int                          argc_;
	char**                       argv_;
	std::stack<AbstractFunctor*> finalize_functors_;
	std::set<ArgValMap*>         argvmap_set_;
	ArgValMap*                   argvmap_global_;
	std::set<Test*>              test_set_;
	int                          numthreads_;
	pthread_mutex_t              mutex_;
};


inline int 
TestFramework::ParseArg(int argc, char** argv)
{
	extern char  *optarg;
	extern int   optind;
	extern int   opterr;
	char         ch;
	
	::optind = 1;
	::opterr = 0;
	while ((ch = getopt(argc, argv, "T:"))!=-1) {
		switch (ch) {
			case 'T':
				{
					std::string argstr(::optarg);
					std::string substr;
					std::string strk;
					std::string strv;
					size_t      p;
					size_t      np;

					if (::optarg[0] == ',') {
						ArgValMap* argval = new ArgValMap(std::string(&::optarg[1]));
						argvmap_set_.insert(argval);
					}
				}
				break;
		}
	}
	return 0;
}


inline
TestFramework::TestFramework(int argc, char** argv)
	: argc_(argc),
	  argv_(argv),
	  argvmap_global_(NULL)
{
	int nthreads = 0;

	pthread_mutex_init(&mutex_, NULL);
	ParseArg(argc, argv);
	for (std::set<ArgValMap*>::iterator it = argvmap_set_.begin();
	     it != argvmap_set_.end();
		 ++it)
	{
		ArgValMap* argvmap = *it;
		if (argvmap->ArgVal(std::string("thread"))) {
			++nthreads;			
		} else {
			// merge all non thread arguments under a single global map
			if (argvmap_global_) {
				argvmap_global_->Merge(*argvmap, false);
			} else {
				argvmap_global_ = argvmap;
			}
		}
	}

	if (nthreads > 0) {
		for (std::set<ArgValMap*>::iterator it = argvmap_set_.begin();
			 it != argvmap_set_.end();
			 ++it)
		{
			ArgValMap* argvmap = *it;
			if (argvmap->ArgVal(std::string("thread"))) {
				Test* t = new Test(argvmap_global_, argvmap);
				test_set_.insert(t);
			}
		}
	} else {
		Test* t = new Test(argvmap_global_, NULL);
		test_set_.insert(t);
	}
}


inline int
TestFramework::RunTests()
{
	int ret;
	
	numthreads_ = test_set_.size();
	for (std::set<Test*>::iterator it = test_set_.begin();
		 it != test_set_.end();
		 ++it)
	{
		// run each test under a separate thread
		// since we match against a filter, multiple unit tests may end up 
		// running under a test thread, which is fine.
		Test* t = *it;
		pthread_create(&t->thread_, NULL, Test::ThreadEntryPoint, (void*) t);
	}
	
	// wait for all threads to finish
	for (std::set<Test*>::iterator it = test_set_.begin();
		 it != test_set_.end();
		 ++it)
	{
		Test* t = *it;
		pthread_join(t->thread_, NULL);
	}
	// now do any cleanup requested by tests by executing finalize functors
	// in reverse order (stack)
	while (!finalize_functors_.empty())
	{
		AbstractFunctor* functor = finalize_functors_.top();
		finalize_functors_.pop();
		(*functor)();
	}

	// redirect each deferred output to standard error
	for (std::set<Test*>::iterator it = test_set_.begin();
		 it != test_set_.end();
		 ++it)
	{
		Test* t = *it;
		if (t->deferred_) {
			std::cerr << t->oss_.str() << std::endl;
		}
	}
	return 0;
}

inline 
Test* TestFramework::tid2Test(pthread_t tid)
{
	for (std::set<Test*>::iterator it = test_set_.begin();
		 it != test_set_.end();
		 ++it)
	{
		Test* t = *it;
		if (t->thread_ == tid) {
			return t;
		}
	}
	return NULL;
}


inline int 
TestFramework::ArgVal(std::string& key, std::string& val)
{
	if (argvmap_global_) {
		return argvmap_global_->ArgVal(key, val);
	}
	return -1;
}


inline int 
TestFramework::ArgVal(const char* key, std::string& val)
{
	std::string keystr(key);
	return ArgVal(keystr, val);
}

inline void
TestFramework::RegisterFinalize(AbstractFunctor* functor)
{
	pthread_mutex_lock(&mutex_);
	finalize_functors_.push(functor);
	pthread_mutex_unlock(&mutex_);
}

}


#endif /* _TESTFW_TESTFW_H_APK199 */
