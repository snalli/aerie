#ifndef _REGISTRY_SERVER_H_IKL111
#define _REGISTRY_SERVER_H_IKL111

#include <pthread.h>
#include <stdint.h>
#include <map>
#include <string>


class RegistryServer {
	public:
		RegistryServer();
		int Init();
		int Lookup(std::string, uint64_t*);
		int Add(std::string, uint64_t);
		int Remove(std::string);
	private:
		pthread_mutex_t                 mutex_;
		std::map<std::string, uint64_t> map_;
};


#endif
