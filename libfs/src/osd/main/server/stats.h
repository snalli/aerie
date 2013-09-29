#ifndef __STAMNOS_OSD_SERVER_STATISTICS_H
#define __STAMNOS_OSD_SERVER_STATISTICS_H

#include <stdint.h>

namespace osd {
namespace server {

// this class is a Singleton because we need register a signal handler that 

#define STATISTICS_EVENTS(ACTION)                                                           \
   ACTION(publish, "Publish events", NULL)                                                  \
   ACTION(publish_time, "Cycles spent in publish", NULL)                                    \
   ACTION(txcommits, "Transactions committed", NULL)                                        \
   ACTION(clflushes, "Cachelines flushed", NULL)                                            
//   ACTION(txclflushes, "Cachelines flushed per transaction", Average(clflushes, txcommits))


#ifdef _STAMNOS_EXPAND_STATISTICS
# define STATISTICS_INC(event) Statistics::instance()->Increment(Statistics::event, 1);
# define STATISTICS_INC_BY(event, val) Statistics::instance()->Increment(Statistics::event, val);
#else
# define STATISTICS_INC(event)
# define STATISTICS_INC_BY(event, val)
#endif


class Statistics {
public:
	#define ENUMERATE_EVENTS(event, description, computefunction) event, 
	enum {
		kNullEvent = -1,
		STATISTICS_EVENTS(ENUMERATE_EVENTS)
		kEventsCount
	};
	#undef ENUMERATE_EVENTS
	
	Statistics();

	static int Create();
	static void SignalHandler(int signo);
	static Statistics* instance();
	int Init();
	void Report();
	void Increment(int event, uint64_t val) {
		event_counts_[event]+=val;
	}
private:
	uint64_t Average(int event1, int event2);

	uint64_t           event_counts_[kEventsCount];
	static Statistics* instance_;
};

} // namespace server
} // namespace osd

#endif /* __STAMNOS_OSD_SERVER_STATISTICS_H */
