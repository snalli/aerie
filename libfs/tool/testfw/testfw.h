#ifndef _TESTFW_TESTFW_H_APK199
#define _TESTFW_TESTFW_H_APK199

#include "tool/testfw/argvmap.h"

#define TESTFW (testfw::__testfwp)


namespace testfw {

class TestFramework;

extern TestFramework* __testfwp;

class TestFramework {
public:
	TestFramework(int, char**);
	
	int ArgVal(std::string& key, std::string& val);
	int ArgVal(const char*, std::string& val);
	const char* SuiteName();
	const char* TestName();
	const char* Tag();

private:
	int        argc_;
	char**     argv_;
	ArgValMap  argvmap_;
};


inline
TestFramework::TestFramework(int argc, char** argv)
	: argc_(argc),
	  argv_(argv),
	  argvmap_(argc, argv)
{

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


inline const char*
TestFramework::SuiteName()
{
	std::string* val;

	if (val=argvmap_.ArgVal(std::string("suite"))) {
		return val->c_str();
	}
	return NULL;
}


inline const char*
TestFramework::TestName()
{
	std::string* val;

	if (val=argvmap_.ArgVal(std::string("test"))) {
		return val->c_str();
	}
	return NULL;
}


inline const char*
TestFramework::Tag()
{
	std::string* val;

	if (val=argvmap_.ArgVal(std::string("tag"))) {
		return val->c_str();
	}
	return NULL;
}


}


#endif /* _TESTFW_TESTFW_H_APK199 */
