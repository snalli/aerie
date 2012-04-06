#ifndef _TESTFW_CMD_LINE_ARGS_H_AKL189
#define _TESTFW_CMD_LINE_ARGS_H_AKL189

#include <string>
#include <assert.h>
#include <unistd.h>
#include <map>
#include <ostream>

namespace testfw {

class ArgValMap {
public:
	ArgValMap(std::string);
	int ArgVal(std::string& key, std::string& val);
	std::string* ArgVal(std::string key);
	int Merge(ArgValMap& other, bool overwrite);
	void Print(std::ostream& os);

private:
	int Init(std::string argstr);

	std::map<std::string, std::string> argvmap_;
};


inline ArgValMap::ArgValMap(std::string argstr)
{
	assert(Init(argstr)==0);
}


inline int 
extract_kv(std::string& str, std::string& strk, std::string& strv)
{
	size_t  start;
	size_t  end;

	start=str.find('-', 0);
	if (start != std::string::npos) {
		end=str.find('=', 0);
		strk=str.substr(start+1, end-1);
		if (end != std::string::npos) {
			strv=str.substr(end+1, std::string::npos);
		}
	}

	return 0;
}


inline int 
ArgValMap::Init(std::string argstr)
{
	std::string substr;
	std::string strk;
	std::string strv;
	size_t      p;
	size_t      np;

	p = 0;
	for(;;) {
		np = argstr.find(',', p);
		if (np != std::string::npos) {
			substr = argstr.substr(p, np-p);
			extract_kv(substr, strk, strv);
			if (!strk.empty()) {
				argvmap_[strk] = strv;
			}
		} else {
			substr = argstr.substr(p);
			extract_kv(substr, strk, strv);
			if (!strk.empty()) {
				argvmap_[strk] = strv;
			}
			break;
		}
		p = np+1;
	}
	return 0;
}


inline int 
ArgValMap::Merge(ArgValMap& other, bool overwrite)
{
	std::map<std::string, std::string>::iterator it;

	for (it = other.argvmap_.begin(); it != other.argvmap_.end(); ++it) {
		std::string key = it->first;
		std::string val = it->second;
		if (argvmap_.find(key) != argvmap_.end()) {
			if (overwrite) {
				argvmap_[key] = val;
			}
		} else {
			argvmap_[key] = val;
		}
	}
	return 0;
}


inline int 
ArgValMap::ArgVal(std::string& key, std::string& val)
{
	if (argvmap_.find(key) != argvmap_.end()) {
		val = argvmap_[key];
		return 0;
	}
	return -1;
}

inline std::string*
ArgValMap::ArgVal(std::string key)
{
	if (argvmap_.find(key) != argvmap_.end()) {
		return &argvmap_[key];
	}
	return NULL;
}


inline void 
ArgValMap::Print(std::ostream& os)
{
	std::map<std::string, std::string>::iterator it;

	for (it = argvmap_.begin(); it != argvmap_.end(); ++it) {
		std::string key = it->first;
		std::string val = it->second;
		os << key << ":" << val << std::endl;
	}
}



}


#endif /* _TESTFW_CMD_LINE_ARGS_H_AKL189 */
