#ifndef _TESTFW_TESTFW_H_APK199
#define _TESTFW_TESTFW_H_APK199

#include "tool/testfw/unittest.h"
#include "tool/testfw/argvmap.h"

#define TESTFW (testfw::__testfwp)


namespace testfw {

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

private:
	int         argc_;
	char**      argv_;
	ArgValMap   argvmap_;
	bool        interactive_;
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

	if (test_name_) {
		RunTestIfNameIs predicate(test_name_);
		ret = runner.RunTestsIf(UnitTest::Test::GetTestList(), suite_name_, predicate, 0);
	} else {
		ret = runner.RunTestsIf(UnitTest::Test::GetTestList(), suite_name_, UnitTest::True(), 0);
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
