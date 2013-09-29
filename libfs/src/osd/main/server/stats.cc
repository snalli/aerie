#include "osd/main/server/stats.h"
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "common/errno.h"
#include "bcs/main/common/cdebug.h"


namespace osd {
namespace server {

Statistics* Statistics::instance_ = NULL;

Statistics::Statistics()
{

}

int 
Statistics::Create()
{
	if (Statistics::instance_) {
		return E_SUCCESS;
	}
	if ((Statistics::instance_ = new Statistics()) == NULL) {
		return -E_ERROR;
	}
	return Statistics::instance()->Init();
} 

int 
Statistics::Init()
{
	if (signal(SIGUSR1, Statistics::SignalHandler) == SIG_ERR) {
		DBG_LOG(DBG_CRITICAL, DBG_MODULE(server_statistics), "Cannot register signal handler\n", clt);	
		return -E_ERROR;
	}
	for (int i=0; i < kEventsCount; i++) {
		event_counts_[i] = 0;
	}
	return E_SUCCESS;
}

Statistics* Statistics::instance()
{
	return Statistics::instance_;
}

void Statistics::SignalHandler(int signo)
{
	if (signo == SIGUSR1) {
		Statistics::instance()->Report();
	}
}

#define EVENT_DESCRIPTION(event, description, compute_function)                   \
   do {                                                                           \
      void* ret;                                                                  \
      if (ret = (void*) compute_function) {                                       \
          std::cerr << description << " = " << (uint64_t) ret << std::endl;       \
      } else  {                                                                   \
          std::cerr << description << " = " << event_counts_[event] << std::endl; \
      }                                                                           \
   } while (0);

void Statistics::Report()
{
#ifdef _STAMNOS_EXPAND_STATISTICS
	std::cerr << "Statistics" << std::endl;
		
	STATISTICS_EVENTS(EVENT_DESCRIPTION)
#else
	std::cerr << "Not built with runtime statistics support!" << std::endl;
#endif
}

#undef EVENT_DESCRIPTION

uint64_t Statistics::Average(int event1, int event2) 
{ 
	uint64_t sum = event_counts_[event1];
	uint64_t n = event_counts_[event2];

	if (n) { 
		return sum/n;
	}
	return 0;
}



} // namespace server
} // namespace osd
