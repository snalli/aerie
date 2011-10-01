#ifndef _TESTFW_CMD_LINE_ARGS_H_AKL189
#define _TESTFW_CMD_LINE_ARGS_H_AKL189

#include <assert.h>
#include <unistd.h>
#include <map>

namespace testfw {

class ArgValMap {
public:
	ArgValMap(int, char**);
	int ArgVal(std::string& key, std::string& val);
	std::string* ArgVal(std::string key);

private:
	int                                Init(int, char**);
	std::map<std::string, std::string> argvmap_;
};

inline ArgValMap::ArgValMap(int argc, char** argv)
{
	assert(Init(argc, argv)==0);
	std::map<std::string, std::string>::iterator it;
}


inline int 
extract_kv(std::string& str, std::string& strk, std::string& strv)
{
	char k[128];
	char v[128];
	int  start;
	int  end;

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
ArgValMap::Init(int argc, char** argv)
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
						p = 1;
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
					}
				}
				break;
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


}


#endif /* _TESTFW_CMD_LINE_ARGS_H_AKL189 */
