#ifndef _TESTFW_TESTFW_H_APK199
#define _TESTFW_TESTFW_H_APK199

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unittest++/UnitTest++.h>
#include <unittest++/TestReporterStdout.h>
#include <unittest++/TestRunner.h>
#include <unittest++/Test.h>
#include <unittest++/XmlTestReporter.h>
#include "tool/testfw/unittest.h"
#include "tool/testfw/argvmap.h"

#define TESTFW (testfw::__testfwp)


namespace testfw {

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




class TestFramework;

extern TestFramework* __testfwp;

class TestFramework {
public:
	TestFramework(int, char**);
	
	inline int RunTests();
	int ArgVal(std::string& key, std::string& val);
	int ArgVal(const char*, std::string& val);
	const char* SuiteName() { return suite_name_; }
	const char* TestName() { return test_name_; }
	const char* Tag() { return tag_name_; }
	int numclients() { return numclients_; }

private:
	int         argc_;
	char**      argv_;
	ArgValMap   argvmap_;
	bool        interactive_;
	int         numclients_;
	const char* suite_name_;  
	const char* test_name_;  
	const char* tag_name_;  
};


inline
TestFramework::TestFramework(int argc, char** argv)
	: argc_(argc),
	  argv_(argv),
	  argvmap_(argc, argv)
{
	std::string* val;

	for (int i = argc; i<argc;i++) {
		printf("arg=%s\n", argv[i]);
	}

	if (val = argvmap_.ArgVal(std::string("test"))) {
		test_name_ = val->c_str();
	} else {
		test_name_ = NULL;
	}

	if (val = argvmap_.ArgVal(std::string("suite"))) {
		suite_name_ = val->c_str();
	} else {
		suite_name_ = NULL;
	}

	if (val = argvmap_.ArgVal(std::string("tag"))) {
		tag_name_ = val->c_str();
	} else {
		tag_name_ = NULL;
	}
	
	if (val = argvmap_.ArgVal(std::string("numclients"))) {
		numclients_ = atoi(val->c_str());
	} else {
		numclients_ = 0;
	}

	if (val = argvmap_.ArgVal(std::string("interactive"))) {
		interactive_ = true;
	} else {
		interactive_ = false;
	}
}


inline int
TestFramework::RunTests()
{
	int                      ret;
	UnitTest::TestReporter*  reporter;
	
	if (interactive_) {
		reporter = new UnitTest::TestReporterStdout();
	} else {
		reporter = new UnitTest::XmlTestReporter(std::cerr);
	}

	UnitTest::TestRunner     runner(*reporter);

	if (suite_name_ || test_name_) {
		RunTestIfMatchFilter predicate(suite_name_, test_name_);
		ret = runner.RunTestsIf(UnitTest::Test::GetTestList(), NULL, predicate, 0);
	} else {
		ret = runner.RunTestsIf(UnitTest::Test::GetTestList(), NULL, UnitTest::True(), 0);
	}

	if (!interactive_) {
		std::cerr << std::endl; // force a new line after the tests report
	}

	return ret;
}



inline int 
TestFramework::ArgVal(std::string& key, std::string& val)
{
	return argvmap_.ArgVal(key, val);
}


inline int 
TestFramework::ArgVal(const char* key, std::string& val)
{
	std::string keystr(key);
	return ArgVal(keystr, val);
}


}


#endif /* _TESTFW_TESTFW_H_APK199 */
